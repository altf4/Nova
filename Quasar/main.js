// Used for debugging. Download the node-segfault-handler to use
//var segvhandler = require('./node_modules/segvcatcher/lib/segvhandler')
//segvhandler.registerHandler();

//var agent = require('webkit-devtools-agent');

/*var memwatch = require('memwatch');

memwatch.on('leak', function(info) {
    console.log(info);
});

var hd = new memwatch.HeapDiff();
memwatch.on('stats', function(stats) {
    console.log(stats);
    var diff = hd.end();

    console.log(diff.change.details);
    //console.log(diff);
    console.log("\n\n");

    hd = new memwatch.HeapDiff();
});
*/

// Modules that provide bindings to C++ code in NovaLibrary and Nova_UI_Core
var novaconfig = require('novaconfig.node');

var nova = new novaconfig.Instance();
nova.CheckConnection();

var config = new novaconfig.NovaConfigBinding();
var honeydConfig = new novaconfig.HoneydConfigBinding();
var vendorToMacDb = new novaconfig.VendorMacDbBinding();
var osPersonalityDb = new novaconfig.OsPersonalityDbBinding();
var trainingDb = new novaconfig.CustomizeTrainingBinding();
var whitelistConfig = new novaconfig.WhitelistConfigurationBinding();
var hhconfig = new novaconfig.HoneydAutoConfigBinding();

// Modules from NodejsModule/Javascript
var LOG = require("../NodejsModule/Javascript/Logger").LOG;


if (!honeydConfig.LoadAllTemplates()) {
	LOG("ERROR", "Call to initial LoadAllTemplates failed!");
}

var fs = require('fs');
var path = require('path');
var jade = require('jade');
var express = require('express');
var util = require('util');
var https = require('https');
var passport = require('passport');
var BasicStrategy = require('passport-http').BasicStrategy;
var sql = require('sqlite3');
var validCheck = require('validator').check;
var sanitizeCheck = require('validator').sanitize;
var crypto = require('crypto');
var exec = require('child_process').exec;
var nowjs = require("now");
var Validator = require('validator').Validator;
var dns = require('dns');

var Tail = require('tail').Tail;
var NovaHomePath = config.GetPathHome();
var NovaSharedPath = config.GetPathShared();
var novadLogPath = "/var/log/nova/Nova.log";
var novadLog = new Tail(novadLogPath);

var honeydLogPath = "/var/log/nova/Honeyd.log";
var honeydLog = new Tail(honeydLogPath);

var benignRequest = false;
var mothership;

var RenderError = function (res, err, link) {
	// Redirect them to the main page if no link was set
	link = typeof link !== 'undefined' ? link : "/";

	console.log("Reported Client Error: " + err);
	res.render('error.jade', {
		locals: {
			redirectLink: link
			, errorDetails: err
		}
	});
}

var HashPassword = function (password) {
	var shasum = crypto.createHash('sha1');
	shasum.update(password);
	return shasum.digest('hex');
}

LOG("ALERT", "Starting QUASAR version " + config.GetVersionString());


process.chdir(NovaHomePath);

var DATABASE_HOST = config.ReadSetting("DATABASE_HOST");
var DATABASE_USER = config.ReadSetting("DATABASE_USER");
var DATABASE_PASS = config.ReadSetting("DATABASE_PASS");

var databaseOpenResult = function (err) {
	if (err == null) {
		console.log("Opened sqlite3 database file.");
	} else {
		LOG(ERROR, "Error opening sqlite3 database file: " + err);
	}
}

var db = new sql.Database(NovaHomePath + "/data/database.db", sql.OPEN_READWRITE, databaseOpenResult);


// Prepare query statements
var dbqCredentialsRowCount = db.prepare('SELECT COUNT(*) AS rows from credentials');
var dbqCredentialsCheckLogin = db.prepare('SELECT user, pass FROM credentials WHERE user = ? AND pass = ?');
var dbqCredentialsGetUsers = db.prepare('SELECT user FROM credentials');
var dbqCredentialsGetUser = db.prepare('SELECT user FROM credentials WHERE user = ?');
var dbqCredentialsChangePassword = db.prepare('UPDATE credentials SET pass = ? WHERE user = ?');
var dbqCredentialsInsertUser = db.prepare('INSERT INTO credentials VALUES(?, ?)');
var dbqCredentialsDeleteUser = db.prepare('DELETE FROM credentials WHERE user = ?');

var dbqFirstrunCount = db.prepare("SELECT COUNT(*) AS rows from firstrun");
var dbqFirstrunInsert = db.prepare("INSERT INTO firstrun values(datetime('now'))");

var dbqSuspectAlertsGet = db.prepare('SELECT suspect_alerts.id, timestamp, suspect, classification, ip_traffic_distribution,port_traffic_distribution,packet_size_mean,packet_size_deviation,distinct_ips,distinct_tcp_ports,distinct_udp_ports,avg_tcp_ports_per_host,avg_udp_ports_per_host,tcp_percent_syn,tcp_percent_fin,tcp_percent_rst,tcp_percent_synack,haystack_percent_contacted FROM suspect_alerts LEFT JOIN statistics ON statistics.id = suspect_alerts.statistics');
var dbqSuspectAlertsDeleteAll = db.prepare('DELETE FROM suspect_alerts');
var dbqSuspectAlertsDeleteAlert = db.prepare('DELETE FROM suspect_alerts where id = ?');

passport.serializeUser(function (user, done) {
	done(null, user);
});

passport.deserializeUser(function (user, done) {
	done(null, user);
});

passport.use(new BasicStrategy(

function (username, password, done) {
	var user = username;
	process.nextTick(function () {

		dbqCredentialsRowCount.all(function (err, rowcount) {
			if (err) {
				console.log("Database error: " + err);
			}

			// If there are no users, add default nova and log in
			if (rowcount[0].rows == 0) {
				console.log("No users in user database. Creating default user.");
				dbqCredentialsInsertUser.run('nova', HashPassword('toor'), function (err) {
					if (err) {
						throw err;
					}

					switcher(err, user, true, done);

				});
			} else {
				dbqCredentialsCheckLogin.all(user, HashPassword(password),

				function selectCb(err, results) {
					if (err) {
						console.log("Database error: " + err);
					}

					if (results[0] === undefined) {
						switcher(err, user, false, done);
					} else if (user === results[0].user) {
						switcher(err, user, true, done);
					} else {
						switcher(err, user, false, done);
					}
				});
			}
		});
	});
}));

// Setup TLS
var express_options = {
	key: fs.readFileSync(NovaHomePath + '/config/keys/quasarKey.pem'),
	cert: fs.readFileSync(NovaHomePath + '/config/keys/quasarCert.pem')
};

var app = express.createServer(express_options);

app.configure(function () {
	app.use(passport.initialize());
	app.use(express.bodyParser());
	app.use(express.cookieParser());
	app.use(express.methodOverride());
	app.use(app.router);
	app.use(express.static(NovaSharedPath + '/Quasar/www'));
});

app.set('views', __dirname + '/views');

app.set('view options', {
	layout: false
});

// Do the following for logging
//console.info("Logging to ./serverLog.log");
//var logFile = fs.createWriteStream('./serverLog.log', {flags: 'a'});
//app.use(express.logger({stream: logFile}));

var WEB_UI_PORT = config.ReadSetting("WEB_UI_PORT");
console.info("Listening on port " + WEB_UI_PORT);
app.listen(WEB_UI_PORT);
var everyone = nowjs.initialize(app);


var initLogWatch = function () {
	var novadLog = new Tail(novadLogPath);
	novadLog.on("line", function (data) {
		try {
			everyone.now.newLogLine(data);
		} catch (err) {

		}
	});

	novadLog.on("error", function (data) {
		LOG(ERROR, "Novad log watch error: " + data);
		try {
			everyone.now.newLogLine(data)
		} catch (err) {
			LOG(ERROR, "Novad log watch error: " + err);
		}
	});


	var honeydLog = new Tail(honeydLogPath);
	honeydLog.on("line", function (data) {
		try {
			everyone.now.newHoneydLogLine(data);
		} catch (err) {

		}
	});

	honeydLog.on("error", function (data) {
		LOG(ERROR, "Honeyd log watch error: " + data);
		try {
			everyone.now.newHoneydLogLine(data)
		} catch (err) {
			LOG(ERROR, "Honeyd log watch error: " + err);

		}
	});
}

initLogWatch();

if(config.ReadSetting('MASTER_UI_ENABLED') === '1')
{
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // SOCKET STUFF FOR MOTHERSHIP 
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Thoughts: Probably going to need a global variable for the websocket connection to
  // the mothership; an emit needs to be made to the mothership in certain circumstances,
  // such as recieving a hostile suspect, so having that be accessible to the nowjs methods
  // is ideal. Also need to find a way to maintain IP address of Mothership and any needed
  // credentials for reboots, etc. 
  
  // TODO: Make configurable
  var connected = config.ReadSetting('MASTER_UI_IP') + ':8081';
  
  var WebSocketClient = require('websocket').client;
  var client = new WebSocketClient();
  // TODO: Make configurable
  var clientId = config.ReadSetting('MASTER_UI_CLIENT_ID');
  var reconnecting = false;
  var clearReconnect;
  var reconnectTimer = parseInt(config.ReadSetting('MASTER_UI_RECONNECT_TIME')) * 1000;
  
  // If the connection fails, print an error message
  client.on('connectFailed', function(error)
  {
    console.log('Connect to Mothership error: ' + error.toString());
    if(!reconnecting)
    {
      console.log('No current attempts to reconnect, starting reconnect attempts every ', (reconnectTimer / 1000) ,' seconds.');
      // TODO: Don't have static lengths for reconnect interval; make configurable
      clearReconnect = setInterval(function(){console.log('attempting reconnect to wss://' + connected); client.connect('wss://' + connected, null);}, reconnectTimer);
      reconnecting = true;
    }
  });
  
  // If we successfully connect to the mothership, perform event-based actions here.
  // The interface for websockets is a little less nice than socket.io, simply because
  // you cannot name your own events, everything has to be done within the message
  // directive; it's only one more level of indirection, however, so no big deal
  client.on('connect', function(connection){
    // When the connection is established, we save the connection variable
    // to the mothership global so that actions can be taken on the connection
    // outside of this callback if needed (hostile suspect, etc.)
    reconnecting = false;
    clearInterval(clearReconnect);
    mothership = connection;
    var quick = {};
    quick.type = 'addId';
    quick.id = clientId;
    quick.nova = nova.IsNovadUp(false).toString();
    quick.haystack = nova.IsHaystackUp(false).toString();
    quick.benignRequest = (benignRequest == true ? 'true' : 'false');
    // I don't know that we HAVE to use UTF8 here, there's a send() method as 
    // well as a 'data' member inside the message objects instead of utf8Data.
    // But, as it was in the Websockets tutorial Pherric found, we'll use it for now
    mothership.sendUTF(JSON.stringify(quick));
    console.log('Registering client Id with mothership');
    
    var configSend = {};
    configSend.type = 'registerConfig';
    configSend.id = clientId;
    configSend.filename = 'NOVAConfig@' + clientId + '.txt';
    configSend.file = fs.readFileSync(NovaHomePath + '/config/NOVAConfig.txt', 'utf8');
    mothership.sendUTF(JSON.stringify(configSend));
    console.log('Registering NOVAConfig with mothership');
  
    var interfaceSend = {};
    interfaceSend.type = 'registerClientInterfaces';
    interfaceSend.id = clientId;
    interfaceSend.filename = 'iflist@' + clientId + '.txt';
    interfaceSend.file = config.ListInterfaces().sort().join();
    mothership.sendUTF(JSON.stringify(interfaceSend));
    console.log('Registering interfaces with mothership');
  
    console.log('successfully connected to mothership');
  
    connection.on('message', function(message)
    {
      if(message.type === 'utf8')
      {
        // We send the messages as JSON, so we have to parse it out
        var json_args = JSON.parse(message.utf8Data);
        if(json_args != undefined)
        {
          // the various actions to take when a message is received
          switch(json_args.type)
          {
            case 'startNovad':
              nova.StartNovad(false);
              nova.CheckConnection();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Novad is being started on ' + clientId;
              mothership.sendUTF(JSON.stringify(response));
              break;
            case 'stopNovad':
              nova.StopNovad();
              nova.CloseNovadConnection();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Novad is being stopped on ' + clientId;
              mothership.sendUTF(JSON.stringify(response));
              break;
            case 'startHaystack':
              if(!nova.IsHaystackUp())
              {
                nova.StartHaystack(false);
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Haystack is being started on ' + clientId;
                mothership.sendUTF(JSON.stringify(response));
              }
              else
              {
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Haystack is already up, doing nothing';
                mothership.sendUTF(JSON.stringify(response));
              }
              break;
            case 'stopHaystack':
              nova.StopHaystack();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Haystack is being stopped on ' + clientId;
              mothership.sendUTF(JSON.stringify(response));
              break;
            case 'writeSetting':
              // The nice thing about using JSON for the message passing is we
              // can name the object literal members whatever we want, allowing for
              // implicit specificity when creating and parsing the object
              var setting = json_args.setting;
              var value = json_args.value;
              config.WriteSetting(setting, value);
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = setting + ' is now ' + value;
              mothership.sendUTF(JSON.stringify(response));
              break;
            case 'getHostileSuspects':
              nova.sendSuspectList(distributeSuspect);
              break;
            case 'requestBenign':
              benignRequest = true;
              nova.sendSuspectList(distributeSuspect);
              break;
            case 'cancelRequestBenign':
              benignRequest = false;
              break;
            case 'updateConfiguration':
              for(var i in json_args)
              {
                if(i !== 'type' && i !== 'id')
                {
                  config.WriteSetting(i, json_args[i]);
                }
              }
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Configuration for ' + clientId + ' has been updated. Registering new config...';
              mothership.sendUTF(JSON.stringify(response));
              configSend.file = fs.readFileSync(NovaHomePath + '/config/NOVAConfig.txt', 'utf8');
              mothership.sendUTF(JSON.stringify(configSend));
              break;
            case 'haystackConfig':
              var executionString = 'haystackautoconfig';
              var nFlag = '-n';
              var rFlag = '-r';
              var iFlag = '-i';
            
              var hhconfigArgs = new Array();
            
              if(json_args.numNodesType == "number") 
              {
                if(json_args.numNodes !== undefined) 
                {
                  hhconfigArgs.push(nFlag);
                  hhconfigArgs.push(json_args.numNodes);
                }
              } 
              else if(json_args.numNodesType == "ratio") 
              {
                if(json_args.numNodes !== undefined) 
                {
                  hhconfigArgs.push(rFlag);
                  hhconfigArgs.push(json_args.numNodes);
                }
              }
              if(json_args.interface !== undefined && json_args.interface.length > 0) {
                hhconfigArgs.push(iFlag);
                hhconfigArgs.push(json_args.interface);
              }
            
              var util = require('util');
              var spawn = require('child_process').spawn;
            
              var autoconfig = spawn(executionString.toString(), hhconfigArgs);
            
              autoconfig.stdout.on('data', function (data){
                console.log('' + data);
              });
            
              autoconfig.stderr.on('data', function (data){
                if (/^execvp\(\)/.test(data)) {
                  console.log("haystackautoconfig failed to start.");
                  var message = "haystackautoconfig failed to start.";
                  var response = {};
                  response.id = clientId;
                  response.type = 'response';
                  response.response_message = message;
                  mothership.sendUTF(JSON.stringify(response));
                }
              });
            
              autoconfig.on('exit', function (code){
                console.log("autoconfig exited with code " + code);
                var message = "autoconfig exited with code " + code;
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = message;
                mothership.sendUTF(JSON.stringify(response));
              });

              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Haystack Autoconfiguration commencing';
              mothership.sendUTF(JSON.stringify(response));
              break;
            case 'suspectDetails':
              var suspectString = nova.GetSuspectDetailsString(json_args.ip, json_args.iface);
              var response = {};
              response.id = clientId;
              response.type = 'detailsReceived';
              response.data = suspectString;
              mothership.sendUTF(JSON.stringify(response));
              break;
            case 'clearSuspects':
              everyone.now.ClearAllSuspects(function(){
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Suspects cleared on ' + json_args.id;
                mothership.sendUTF(JSON.stringify(response));
              });
              break;
            default:
              console.log('Unexpected/unknown message type ' + json_args.type + ' received, doing nothing');
              break;
          }
        }
      }
    });
    connection.on('close', function(){
      // If the connection gets closed, we want to try to reconnect; we will use
      // the stored IP of the Mothership to make the reconnect attempts
      mothership = undefined;
      if(!reconnecting)
      {
        console.log('closed, beginning reconnect attempts every ', (reconnectTimer / 1000) ,' seconds');
        clearReconnect = setInterval(function(){console.log('attempting reconnect to wss://' + connected); client.connect('wss://' + connected, null);}, reconnectTimer);
        reconnecting = true;
      }
    });
    connection.on('error', function(error){
      console.log('Error: ' + error.toString());
    });
  });
  
  client.connect('wss://' + connected, null);
  
  setInterval(function() {
    if(mothership != undefined && mothership != null)
    {
      var message1 = {};
      message1.id = clientId;
      message1.type = 'statusChange';
      message1.component = 'nova';
      message1.status = nova.IsNovadUp(false).toString();
      var message2 = {};
      message2.id = clientId;
      message2.type = 'statusChange';
      message2.component = 'haystack';
      message2.status = nova.IsHaystackUp().toString();
      mothership.sendUTF(JSON.stringify(message1));
      mothership.sendUTF(JSON.stringify(message2));
    }
  }, 10000);
  //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

app.get('/downloadNovadLog.log', passport.authenticate('basic', {session: false}), function (req, res) {
	fs.readFile(novadLogPath, 'utf8', function (err, data) {
		if (err) {
			RenderError(res, "Unable to open NOVA log file for reading due to error: " + err);
			return;
		} else {
			// Hacky solution to make browsers launch a save as dialog
			res.header('Content-Type', 'application/novaLog');
			var reply = data.toString();
			res.send(reply);
		}
	});
});

app.get('/downloadHoneydLog.log', passport.authenticate('basic', {session: false}), function (req, res) {
	fs.readFile(honeydLogPath, 'utf8', function (err, data) {
		if (err) {
			RenderError(res, "Unable to open NOVA log file for reading due to error: " + err);
			return;
		} else {
			// Hacky solution to make browsers launch a save as dialog
			res.header('Content-Type', 'application/honeydLog');
			var reply = data.toString();
			res.send(reply);
		}
	});
});

app.get('/novaState.csv', passport.authenticate('basic', {session: false}), function (req, res) {
	exec('novacli get all csv > ' + NovaHomePath + "/state.csv",
	function(error, stdout, stderr) {
		if (error != null) {
			// Don't really care. Probably failed because novad was down.
			//console.log("exec error: " + error);
		}
		
		fs.readFile(NovaHomePath + "/state.csv", 'utf8', function(err, data) {
			res.header('Content-Type', 'text/csv');
			var reply = data.toString();
			res.send(reply);
		});
	});
});

app.get('/viewNovadLog', passport.authenticate('basic', {session: false}), function (req, res) {
	fs.readFile(novadLogPath, 'utf8', function (err, data) {
		if (err) {
			RenderError(res, "Unable to open NOVA log file for reading due to error: " + err);
			return;
		} else {
			var reply = data.toString().split(/(\r\n|\n|\r)/gm);
			var html = "";
			for (var i = 0; i < reply.length; i++) {
				var styleString = "";
				var line = reply[i];
        		var splitLine = line.split(/[\s]+/);
				if (splitLine.length >= 6) {
					if (splitLine[5] == "DEBUG" || splitLine[5] == "INFO") {
						styleString += 'color: green';
					} else if (splitLine[5] == "WARNING" || splitLine[5] == "NOTICE") {
						styleString += 'color: orange';
					} else if (splitLine[5] == "ERROR" || splitLine[5] == "CRITICAL") {
						styleString += 'color: red';
					} else {
						styleString += 'color: blue';
					}
				}

				html += '<P style="' + styleString + '">' + line + "<P>";

			}

			res.render('viewLog.jade', {
				locals: {
					log: html
				}
			});
		}
	});
});

app.get('/viewHoneydLog', passport.authenticate('basic', {session: false}), function (req, res) {
	fs.readFile(honeydLogPath, 'utf8', function (err, data) {
		if (err) {
			RenderError(res, "Unable to open HONEYD log file for reading due to error: " + err);
			return;
		} else {
			var reply = data.toString().split(/(\r\n|\n|\r)/gm);
			var html = "";
			for (var i = 0; i < reply.length; i++) {
				var styleString = "";
				var line = reply[i];
				styleString += 'color: blue';

				html += '<P style="' + styleString + '">' + line + "<P>";

			}

			res.render('viewLog.jade', {
				locals: {
					log: html
				}
			});
		}
	});
});
app.get('/advancedOptions', passport.authenticate('basic', {session: false}), function (req, res) {
	var all = config.ListInterfaces().sort();
	var used = config.GetInterfaces().sort();

	var pass = [];

	for (var i in all) {
		var checked = false;

		for (var j in used) {
			if (all[i] === used[j]) {
				checked = true;
				break;
			}
		}

		if (checked) {
			pass.push({
				iface: all[i],
				checked: 1
			});
		} else {
			pass.push({
				iface: all[i],
				checked: 0
			});
		}
	}

	res.render('advancedOptions.jade', {
		locals: {
			INTERFACES: config.ListInterfaces().sort()
			, DEFAULT: config.GetUseAllInterfacesBinding()
			, HS_HONEYD_CONFIG: config.ReadSetting("HS_HONEYD_CONFIG")
			, TCP_TIMEOUT: config.ReadSetting("TCP_TIMEOUT")
			, TCP_CHECK_FREQ: config.ReadSetting("TCP_CHECK_FREQ")
			, READ_PCAP: config.ReadSetting("READ_PCAP")
			, PCAP_FILE: config.ReadSetting("PCAP_FILE")
			, GO_TO_LIVE: config.ReadSetting("GO_TO_LIVE")
			, CLASSIFICATION_TIMEOUT: config.ReadSetting("CLASSIFICATION_TIMEOUT")
			, K: config.ReadSetting("K")
			, EPS: config.ReadSetting("EPS")
			, CLASSIFICATION_THRESHOLD: config.ReadSetting("CLASSIFICATION_THRESHOLD")
			, DATAFILE: config.ReadSetting("DATAFILE")
			, USER_HONEYD_CONFIG: config.ReadSetting("USER_HONEYD_CONFIG")
			, DOPPELGANGER_IP: config.ReadSetting("DOPPELGANGER_IP")
			, DOPPELGANGER_INTERFACE: config.ReadSetting("DOPPELGANGER_INTERFACE")
			, DM_ENABLED: config.ReadSetting("DM_ENABLED")
			, ENABLED_FEATURES: config.ReadSetting("ENABLED_FEATURES")
			, FEATURE_NAMES: nova.GetFeatureNames()
			, THINNING_DISTANCE: config.ReadSetting("THINNING_DISTANCE")
			, SAVE_FREQUENCY: config.ReadSetting("SAVE_FREQUENCY")
			, DATA_TTL: config.ReadSetting("DATA_TTL")
			, CE_SAVE_FILE: config.ReadSetting("CE_SAVE_FILE")
			, SMTP_ADDR: config.ReadSetting("SMTP_ADDR")
			, SMTP_PORT: config.ReadSetting("SMTP_PORT")
			, SMTP_DOMAIN: config.ReadSetting("SMTP_DOMAIN")
			, SMTP_USER: config.GetSMTPUser()
			, SMTP_PASS: config.GetSMTPPass()
			, RECIPIENTS: config.ReadSetting("RECIPIENTS")
			, SERVICE_PREFERENCES: config.ReadSetting("SERVICE_PREFERENCES")
			, HAYSTACK_STORAGE: config.ReadSetting("HAYSTACK_STORAGE")
			, CAPTURE_BUFFER_SIZE: config.ReadSetting("CAPTURE_BUFFER_SIZE")
			, MIN_PACKET_THRESHOLD: config.ReadSetting("MIN_PACKET_THRESHOLD")
			, CUSTOM_PCAP_FILTER: config.ReadSetting("CUSTOM_PCAP_FILTER")
			, CUSTOM_PCAP_MODE: config.ReadSetting("CUSTOM_PCAP_MODE")
			, WEB_UI_PORT: config.ReadSetting("WEB_UI_PORT")
			, CLEAR_AFTER_HOSTILE_EVENT: config.ReadSetting("CLEAR_AFTER_HOSTILE_EVENT")
			, MASTER_UI_IP: config.ReadSetting("MASTER_UI_IP")
			, MASTER_UI_RECONNECT_TIME: config.ReadSetting("MASTER_UI_RECONNECT_TIME")
			, MASTER_UI_CLIENT_ID: config.ReadSetting("MASTER_UI_CLIENT_ID")
			, MASTER_UI_ENABLED: config.ReadSetting("MASTER_UI_ENABLED") 
			, FEATURE_WEIGHTS: config.ReadSetting("FEATURE_WEIGHTS")
			, CLASSIFICATION_ENGINE: config.ReadSetting("CLASSIFICATION_ENGINE")
			, THRESHOLD_HOSTILE_TRIGGERS: config.ReadSetting("THRESHOLD_HOSTILE_TRIGGERS")
			, ONLY_CLASSIFY_HONEYPOT_TRAFFIC: config.ReadSetting("ONLY_CLASSIFY_HONEYPOT_TRAFFIC")


			, supportedEngines: nova.GetSupportedEngines()
		}
	});
});


function renderBasicOptions(jadefile, res, req) {
	var all = config.ListInterfaces().sort();
	var used = config.GetInterfaces().sort();

	var pass = [];

	for (var i in all) {
		var checked = false;

		for (var j in used) {
			if (all[i] === used[j]) {
				checked = true;
				break;
			}
		}

		if (checked) {
			pass.push({
				iface: all[i],
				checked: 1
			});
		} else {
			pass.push({
				iface: all[i],
				checked: 0
			});
		}
	}

	var doppelPass = [];

	all = config.ListLoopbacks().sort();
	used = config.GetDoppelInterface();

	for (var i in all) {
		var checked = false;

		for (var j in used) {
			if (all[i] === used[j]) {
				checked = true;
				break;
			} else if (used[j].length == 1 && used.length > 1) {
				if (all[i] === used) {
					checked = true;
					break;
				}
			}
		}

		if (checked) {
			doppelPass.push({
				iface: all[i],
				checked: 1
			});
		} else {
			doppelPass.push({
				iface: all[i],
				checked: 0
			});
		}
	}

	res.render(jadefile, {
		locals: {
			INTERFACES: pass,
			DEFAULT: config.GetUseAllInterfacesBinding(),
			DOPPELGANGER_IP: config.ReadSetting("DOPPELGANGER_IP"),
			DOPPELGANGER_INTERFACE: doppelPass,
			DM_ENABLED: config.ReadSetting("DM_ENABLED"),
			SMTP_ADDR: config.ReadSetting("SMTP_ADDR"),
			SMTP_PORT: config.ReadSetting("SMTP_PORT"),
			SMTP_DOMAIN: config.ReadSetting("SMTP_DOMAIN"),
			SMTP_USER: config.GetSMTPUser(),
			SMTP_PASS: config.GetSMTPPass(),
			SMTP_USEAUTH: config.GetSMTPUseAuth().toString(),
			SERVICE_PREFERENCES: config.ReadSetting("SERVICE_PREFERENCES"),
			RECIPIENTS: config.ReadSetting("RECIPIENTS")
		}
	});
}

app.get('/error', passport.authenticate('basic', {session: false}), function (req, res) {
	RenderError(res, req.query["errorDetails"], req.query["redirectLink"]);
	return;
});

app.get('/basicOptions', passport.authenticate('basic', {session: false}), function (req, res) {
	renderBasicOptions('basicOptions.jade', res, req);
});

app.get('/configHoneydNodes', passport.authenticate('basic', {session: false}), function (req, res) {
	if (!honeydConfig.LoadAllTemplates()) {
		RenderError(res, "Unable to load honeyd configuration XML files")
		return;
	}

	var nodeNames = honeydConfig.GetNodeNames();
	var nodes = new Array();

	for (var i = 0; i < nodeNames.length; i++) {
		nodes.push(honeydConfig.GetNode(nodeNames[i]));
	}

	res.render('configHoneyd.jade', {
		locals: {
			INTERFACES: config.ListInterfaces().sort(),
			profiles: honeydConfig.GetProfileNames(),
			nodes: nodes,
			groups: honeydConfig.GetGroups(),
			currentGroup: config.GetGroup()
		}
	})
});


app.get('/configHoneydProfiles', passport.authenticate('basic', {session: false}), function (req, res) {
	if (!honeydConfig.LoadAllTemplates()) {
		RenderError(res, "Unable to load honeyd configuration XML files")
		return;
	}
	
	var profileNames = honeydConfig.GetProfileNames();
	var profiles = {};
	for (var i = 0; i < profileNames.length; i++) {
		profiles[profileNames[i]] = honeydConfig.GetProfile(profileNames[i]);
	}

	res.render('configHoneydProfiles.jade', {
		locals: {
			profileNames: honeydConfig.GetProfileNames(),
			profiles: profiles
		}
	})
});

app.get('/GetSuspectDetails', passport.authenticate('basic', {session: false}), function (req, res) {
	if (req.query["ip"] === undefined) {
		RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/");
		return;
	}
	
	if (req.query["interface"] === undefined) {
		RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/");
		return;
	}
	
	var suspectIp = req.query["ip"];
	var suspectInterface = req.query["interface"];
	var suspectString = nova.GetSuspectDetailsString(suspectIp, suspectInterface);

	res.render('suspectDetails.jade', {
		locals: {
			suspect: suspectIp
			, interface: suspectInterface
			, details: suspectString
		}
	})
});

app.get('/editHoneydNode', passport.authenticate('basic', {session: false}), function (req, res) {
	if (req.query["node"] === undefined) {
		RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/configHoneydNodes");
		return;
	}
	
	var nodeName = req.query["node"];
	var node = honeydConfig.GetNode(nodeName);

	if (node == null) {
		RenderError(res, "Unable to fetch node: " + nodeName, "/configHoneydNodes");
		return;
	}
	res.render('editHoneydNode.jade', {
		locals: {
			oldName: nodeName,
			INTERFACES: config.ListInterfaces().sort(),
			profiles: honeydConfig.GetProfileNames(),
			profile: node.GetProfile(),
			ip: node.GetIP(),
			mac: node.GetMAC()
		}
	})
});

app.get('/editHoneydProfile', passport.authenticate('basic', {session: false}), function (req, res) {
	if (req.query["profile"] === undefined) {
		RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/configHoneydNodes");
		return;
	}
	profileName = req.query["profile"];

	res.render('editHoneydProfile.jade', {
		locals: {
			oldName: profileName,
			parentName: "",
			newProfile: false,
			vendors: vendorToMacDb.GetVendorNames(),
			scripts: honeydConfig.GetScriptNames(),
			personalities: osPersonalityDb.GetPersonalityOptions()
		}
	})
});


app.get('/addHoneydProfile', passport.authenticate('basic', {session: false}), function (req, res) {
	if (req.query["parent"] === undefined) {
		RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/configHoneydNodes");
		return;
	}
	parentName = req.query["parent"];

	res.render('addHoneydProfile.jade', {
		locals: {
			oldName: parentName,
			parentName: parentName,
			newProfile: true,
			vendors: vendorToMacDb.GetVendorNames(),
			scripts: honeydConfig.GetScriptNames(),
			personalities: osPersonalityDb.GetPersonalityOptions()
		}
	})
});

app.get('/customizeTraining', passport.authenticate('basic', {session: false}), function (req, res) {
	trainingDb = new novaconfig.CustomizeTrainingBinding();
	res.render('customizeTraining.jade', {
		locals: {
			desc: trainingDb.GetDescriptions(),
			uids: trainingDb.GetUIDs(),
			hostiles: trainingDb.GetHostile()
		}
	})
});

app.get('/importCapture', passport.authenticate('basic', {session: false}), function (req, res) {
	if (req.query["trainingSession"] === undefined) {
		RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.");
		return;
	}

	var trainingSession = req.query["trainingSession"];
	trainingSession = NovaHomePath + "/data/" + trainingSession + "/nova.dump";
	var ips = trainingDb.GetCaptureIPs(trainingSession);

	if (ips === undefined) {
		RenderError(res, "Unable to read capture dump file");
		return;
	} else {
		res.render('importCapture.jade', {
			locals: {
				ips: trainingDb.GetCaptureIPs(trainingSession),
				trainingSession: req.query["trainingSession"]
			}
		})
	}
});

everyone.now.changeGroup = function(group, callback) {
	callback(config.SetGroup(group));
}

app.post('/importCaptureSave', passport.authenticate('basic', {session: false}), function (req, res) {
	var hostileSuspects = new Array();
	var includedSuspects = new Array();
	var descriptions = new Object();

	var trainingSession = req.query["trainingSession"];
	trainingSession = NovaHomePath + "/data/" + trainingSession + "/nova.dump";

	var trainingDump = new novaconfig.TrainingDumpBinding();
	if (!trainingDump.LoadCaptureFile(trainingSession)) {
		ReadSetting(res, "Unable to parse dump file: " + trainingSession);
	}

	trainingDump.SetAllIsIncluded(false);
	trainingDump.SetAllIsHostile(false);

	for (var id in req.body) {
		id = id.toString();
		var type = id.split('_')[0];
		var ip = id.split('_')[1];

		if (type == 'include') {
			includedSuspects.push(ip);
			trainingDump.SetIsIncluded(ip, true);
		} else if (type == 'hostile') {
			hostileSuspects.push(ip);
			trainingDump.SetIsHostile(ip, true);
		} else if (type == 'description') {
			descriptions[ip] = req.body[id];
			trainingDump.SetDescription(ip, req.body[id]);
		} else {
			console.log("ERROR: Got invalid POST values for importCaptureSave");
		}
	}

	// TODO: Don't hard code this path
	if (!trainingDump.SaveToDb(NovaHomePath + "/config/training/training.db")) {
		RenderError(res, "Unable to save to training db");
		return;
	}

	res.render('saveRedirect.jade', {
		locals: {
			redirectLink: "/customizeTraining"
		}
	})

});

app.get('/configWhitelist', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('configWhitelist.jade', {
		locals: {
			whitelistedIps: whitelistConfig.GetIps(),
			whitelistedRanges: whitelistConfig.GetIpRanges()
		}
	})
});

app.get('/editUsers', passport.authenticate('basic', {session: false}), function (req, res) {
	var usernames = new Array();
	dbqCredentialsGetUsers.all(

	function (err, results) {
		if (err) {
			RenderError(res, "Database Error: " + err);
			return;
		}

		var usernames = new Array();
		for (var i in results) {
			usernames.push(results[i].user);
		}
		res.render('editUsers.jade', {
			locals: {
				usernames: usernames
			}
		});
	});
});

app.get('/configWhitelist', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('configWhitelist.jade', {
		locals: {
			whitelistedIps: whitelistConfig.GetIps(),
			whitelistedRanges: whitelistConfig.GetIpRanges()
		}
	})
});

app.get('/suspects', passport.authenticate('basic', {session: false}), function (req, res) {
	var type;
	if (req.query["type"] == undefined) {
		type = 'all';
	} else {
		type = req.query["type"];
	}

	res.render('main.jade', {
		user: req.user,
		enabledFeatures: config.ReadSetting("ENABLED_FEATURES"),
		featureNames: nova.GetFeatureNames(),
		type: type

	});
});


app.get('/monitor', passport.authenticate('basic', {session: false}), function (req, res) {
	var suspectIp = req.query["ip"];
	var suspectInterface = req.query["interface"];
	
	res.render('monitor.jade', {
		featureNames: nova.GetFeatureNames()
		, suspectIp: suspectIp
		, suspectInterface: suspectInterface
	});
});

app.get('/events', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('events.jade', {
		featureNames: nova.GetFeatureNames()
	});
});

app.get('/novadlog', passport.authenticate('basic', {session: false}), function (req, res) {
	//initLogWatch();
	res.render('novadlog.jade');
});

app.get('/honeydlog', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('honeydlog.jade');
});

app.get('/', passport.authenticate('basic', {session: false}), function (req, res) {
	dbqFirstrunCount.all(

	function (err, results) {
		if (err) {
			RenderError(res, "Database Error: " + err);
			return;
		}

		if (results[0].rows == 0) {
			res.redirect('/welcome');
		} else {
			res.redirect('/suspects');
		}

	});
});

app.get('/createNewUser', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('createNewUser.jade');
});

app.get('/welcome', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('welcome.jade');
});

app.get('/setup1', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('setup1.jade');
});

app.get('/setup2', passport.authenticate('basic', {session: false}), function (req, res) {
	renderBasicOptions('setup2.jade', res, req)
});

app.get('/setup3', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('hhautoconfig.jade', {
		user: req.user,
		INTERFACES: config.ListInterfaces().sort(),
		SCANERROR: ""
	});
});

// Training data capture via Quasar isn't currently supported
//app.get('/CaptureTrainingData', passport.authenticate('basic', {session: false}), function (req, res) {
//	res.render('captureTrainingData.jade');
//});
app.get('/about', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('about.jade');
});

app.get('/haystackStatus', passport.authenticate('basic', {session: false}), function (req, res) {
	fs.readFile("/var/log/honeyd/ipList", 'utf8', function (err, data) {
		var DHCPIps = new Array();
		if (err) {
			RenderError(res, "Unable to open Honeyd status file for reading due to error: " + err);
			return;
		} else {

			data = data.toString().split("\n");
			for(var i = 0; i < data.length; i++) {
				if (data[i] == "") {continue};
				DHCPIps.push(data[i].toString().split(","));
			}	
			res.render('haystackStatus.jade', {
				locals: {
					haystackDHCPIps: DHCPIps
				}
			});
		}
	});
});


app.post('/createNewUser', passport.authenticate('basic', {session: false}), function (req, res) {
	var password = req.body["password"];
	var userName = req.body["username"];
	dbqCredentialsGetUser.all(userName,

	function selectCb(err, results, fields) {
		if (err) {
			RenderError(res, "Database Error: " + err, "/createNewUser");
			return;
		}

		if (results[0] == undefined) {
			dbqCredentialsInsertUser.run(userName, HashPassword(password), function () {
				res.render('saveRedirect.jade', {
					locals: {
						redirectLink: "/"
					}
				});
			});
			return;
		} else {
			RenderError(res, "Username you entered already exists. Please choose another.", "/createNewUser");
			return;
		}
	});
});


app.post('/createInitialUser', passport.authenticate('basic', {session: false}), function (req, res) {
	var password = req.body["password"];
	var userName = req.body["username"];

	dbqCredentialsGetUser.all(userName,

	function selectCb(err, results) {
		if (err) {
			RenderError(res, "Database Error: " + err);
			return;
		}

		if (results[0] == undefined) {
			dbqCredentialsInsertUser.run(userName, HashPassword(password));
			dbqCredentialsDeleteUser.run('nova');
			res.render('saveRedirect.jade', {
				locals: {
					redirectLink: "/setup2"
				}
			});
			return;
		} else {
			RenderError(res, "Username already exists. Please choose another", "/setup1");
			return;
		}
	});
});

app.get('/autoConfig', passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('hhautoconfig.jade', {
		user: req.user,
		INTERFACES: config.ListInterfaces().sort(),
		SCANERROR: ""
	});
});

app.get('/nodeReview', passport.authenticate('basic', {session: false}), function (req, res) {
	if (!honeydConfig.LoadAllTemplates()) {
		RenderError("Unable to load honeyd configuration XML files")
		return;
	}
	
	var profileNames = honeydConfig.GetGeneratedProfileNames();
	var profiles = new Array();
	for (var i = 0; i < profileNames.length; i++) {
		profiles.push(honeydConfig.GetProfile(profileNames[i]));
	}

	var nodeNames = honeydConfig.GetGeneratedNodeNames();
	var nodes = new Array();
	for (var i = 0; i < nodeNames.length; i++) {
		nodes.push(honeydConfig.GetNode(nodeNames[i]));
	}

	res.render('nodereview.jade', {
		locals: {
			profileNames: honeydConfig.GetGeneratedProfileNames(),
			profiles: profiles,
			nodes: nodes,
		}
	})
});

app.get("/editTLSCerts", passport.authenticate('basic', {session: false}), function (req, res) {
	res.render('editTLSCerts.jade');	
});

app.post("/editTLSCerts", passport.authenticate('basic', {session: false}), function (req, res) {
	if (req.files["cert"] == undefined || req.files["key"] == undefined) {
		RenderError(res, "Invalid form submission. This was likely caused by refreshing a page you shouldn't.");
		return;
	}

	if (req.files["cert"].size == 0 || req.files["key"].size == 0) {
		RenderError(res, "You must choose both a key and certificate to upload");
		return;
	}

	fs.readFile(req.files["key"].path, function (readErrKey, data) {
		fs.writeFile(NovaHomePath + "/config/keys/quasarKey.pem", data, function(writeErrKey) {
			
			fs.readFile(req.files["cert"].path, function (readErrCert, certData) {
				fs.writeFile(NovaHomePath + "/config/keys/quasarCert.pem", certData, function(writeErrCert) {
					if (readErrKey != null) {RenderError(res, "Error when reading key file"); return;}
					if (readErrCert != null) {RenderError(res, "Error when reading cert file"); return;}
					if (writeErrKey != null) {RenderError(res, "Error when writing key file"); return;}
					if (writeErrCert != null) {RenderError(res, "Error when writing cert file"); return;}
					
					res.render('saveRedirect.jade', {
						locals: {redirectLink: "/"}
					})
				});
			});
		});
	});
});

app.post('/scanning', passport.authenticate('basic', {session: false}), function (req, res) {
	if (req.body["numNodes"] === undefined) {
		RenderError(res, "Invalid arguements to /scanning. You most likely tried to refresh a page that you shouldn't");
		return;
	}

	var numNodes = req.body["numNodes"];
	var subnets = req.body["subnets"];
	var interfaces = req.body["interfaces"];

	if (interfaces === undefined) {
		interfaces = "";
		console.log("No interfaces selected.");
	}

	if (subnets === undefined) {
		subnets = "";
		console.log("No additional subnets selected.");
	}


	if (!path.existsSync("/usr/bin/haystackautoconfig")) {
		LOG(ERROR, "HaystackAutoConfig binary not found in /usr/bin/. Redirect to /autoConfig.");
		res.render('hhautoconfig.jade', {
			locals: {
				user: req.user,
				INTERFACES: config.ListInterfaces().sort(),
				SCANERROR: "HaystackAutoConfig binary not found, scan cancelled"
			}
		});
	} else if ((subnets === "" && interfaces === "") && (subnets === undefined && interfaces === undefined)) {
		res.redirect('/autoConfig');
	} else {
		hhconfig.RunAutoScan(numNodes, interfaces, subnets);

		res.redirect('/nodeReview');
	}
});

app.post('/customizeTrainingSave', passport.authenticate('basic', {session: false}), function (req, res) {
	for (var uid in req.body) {
		trainingDb.SetIncluded(uid, true);
	}

	trainingDb.Save();

	res.render('saveRedirect.jade', {
		locals: {
			redirectLink: "/customizeTraining"
		}
	})
});

everyone.now.createHoneydNodes = function(ipType, ip1, ip2, ip3, ip4, profile, interface, subnet, count, callback) {
	var ipAddress;
	if (ipType == "DHCP") {
		ipAddress = "DHCP";
	} else {
		ipAddress = ip1 + "." + ip2 + "." + ip3 + "." + ip4;
	}

	var result = null;
	if (!honeydConfig.AddNewNodes(profile, ipAddress, interface, subnet, Number(count))) {
		result = "Unable to create new nodes";	
	}

	if (!honeydConfig.SaveAll()) {
		result = "Unable to save honeyd configuration";
	}

	callback(result);
};

everyone.now.SaveHoneydNode = function(profile, intface, oldName, ipType, macType, ip, mac, callback) {
//app.post('/editHoneydNodeSave', passport.authenticate('basic', {session: false}), function (req, res) {
	var subnet = "";

	var ipAddress = ip;
	if (ipType == "DHCP") {
		ipAddress = "DHCP";
	}

	var macAddress = mac;
	if (macType == "RANDOM") {
		macAddress = "RANDOM";
	}
	
	// Delete the old node and then add the new one	
	honeydConfig.DeleteNode(oldName);
	if (!honeydConfig.AddNewNode(profile, ipAddress, macAddress, intface, subnet)) {
		callback("AddNewNode Failed");
		return;
	} else {
		if (!honeydConfig.SaveAll()) {
			callback("Unable to save honeyd configuration");
		} else {
			callback(null);
		}
	}
};

app.post('/configureNovaSave', passport.authenticate('basic', {session: false}), function (req, res) {
	// TODO: Throw this out and do error checking in the Config (WriteSetting) class instead
	var configItems = ["DEFAULT", "INTERFACE", "SMTP_USER", "SMTP_PASS", "HS_HONEYD_CONFIG", "TCP_TIMEOUT", "TCP_CHECK_FREQ", "READ_PCAP", "PCAP_FILE", "GO_TO_LIVE", "CLASSIFICATION_TIMEOUT", "K", "EPS", "CLASSIFICATION_THRESHOLD", "DATAFILE", "USER_HONEYD_CONFIG", "DOPPELGANGER_IP", "DOPPELGANGER_INTERFACE", "DM_ENABLED", "ENABLED_FEATURES", "THINNING_DISTANCE", "SAVE_FREQUENCY", "DATA_TTL", "CE_SAVE_FILE", "SMTP_ADDR", "SMTP_PORT", "SMTP_DOMAIN", "SMTP_USEAUTH", "RECIPIENTS", "SERVICE_PREFERENCES", "HAYSTACK_STORAGE", "CAPTURE_BUFFER_SIZE", "MIN_PACKET_THRESHOLD", "CUSTOM_PCAP_FILTER", "CUSTOM_PCAP_MODE", "WEB_UI_PORT", "CLEAR_AFTER_HOSTILE_EVENT", "MASTER_UI_IP", "MASTER_UI_RECONNECT_TIME", "MASTER_UI_CLIENT_ID", "MASTER_UI_ENABLED", "CAPTURE_BUFFER_SIZE", "FEATURE_WEIGHTS", "CLASSIFICATION_ENGINE", "THRESHOLD_HOSTILE_TRIGGERS", "ONLY_CLASSIFY_HONEYPOT_TRAFFIC"];

	Validator.prototype.error = function (msg) {
		this._errors.push(msg);
	}

	Validator.prototype.getErrors = function () {
		return this._errors;
	}

	var validator = new Validator();

	config.ClearInterfaces();

  if(req.body["SMTP_USEAUTH"] == undefined)
  {
    req.body["SMTP_USEAUTH"] = "0";
    config.SetSMTPUseAuth("false");
  }
  else
  {
    req.body["SMTP_USEAUTH"] = "1";
    config.SetSMTPUseAuth("true");
  }

  if(clientId != undefined && req.body["MASTER_UI_CLIENT_ID"] != clientId)
  {
    if(mothership != undefined)
    {
      var renameMessage = {};
      renameMessage.type = 'renameRequest';
      renameMessage.id = clientId;
      renameMessage.newId = req.body["MASTER_UI_CLIENT_ID"];
      mothership.sendUTF(JSON.stringify(renameMessage));
      clientId = req.body["MASTER_UI_CLIENT_ID"];
    }
  }

	var interfaces = "";
	var oneIface = false;

	if (req.body["DOPPELGANGER_INTERFACE"] !== undefined) {
		config.SetDoppelInterface(req.body["DOPPELGANGER_INTERFACE"]);
	}

	if (req.body["INTERFACE"] !== undefined) {
		for (item in req.body["INTERFACE"]) {
			if (req.body["INTERFACE"][item].length > 1) {
				interfaces += " " + req.body["INTERFACE"][item];
				config.AddIface(req.body["INTERFACE"][item]);
			} else {
				interfaces += req.body["INTERFACE"][item];
				oneIface = true;
			}
		}

		if (oneIface) {
			config.AddIface(interfaces);
		}

		req.body["INTERFACE"] = interfaces;
	}

	for (var item = 0; item < configItems.length; item++) {
		if (req.body[configItems[item]] == undefined) {
			continue;
		}
		switch (configItems[item]) {
		case "TCP_TIMEOUT":
			validator.check(req.body[configItems[item]], 'Must be a nonnegative integer').isInt();
			break;

		case "WEB_UI_PORT":
			validator.check(req.body[configItems[item]], 'Must be a nonnegative integer').isInt();
			break;

		case "ENABLED_FEATURES":
			validator.check(req.body[configItems[item]], 'Enabled Features mask must be ' + nova.GetDIM() + 'characters long').len(nova.GetDIM(), nova.GetDIM());
			validator.check(req.body[configItems[item]], 'Enabled Features mask must contain only 1s and 0s').regex('[0-1]{9}');
			break;

		case "CLASSIFICATION_THRESHOLD":
			validator.check(req.body[configItems[item]], 'Classification threshold must be a floating point value').isFloat();
			validator.check(req.body[configItems[item]], 'Classification threshold must be a value between 0 and 1').max(1);
			validator.check(req.body[configItems[item]], 'Classification threshold must be a value between 0 and 1').min(0);
			break;

		case "EPS":
			validator.check(req.body[configItems[item]], 'EPS must be a positive number').isFloat();
			break;

		case "THINNING_DISTANCE":
			validator.check(req.body[configItems[item]], 'Thinning Distance must be a positive number').isFloat();
			break;

		case "DOPPELGANGER_IP":
			validator.check(req.body[configItems[item]], 'Doppelganger IP must be in the correct IP format').regex('^((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\.){3}(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})$');
			var split = req.body[configItems[item]].split('.');

			if (split.length == 4) {
				if (split[3] === "0") {
					validator.check(split[3], 'Can not have last IP octet be 0').equals("255");
				}
				if (split[3] === "255") {
					validator.check(split[3], 'Can not have last IP octet be 255').equals("0");
				}
			}

			//check for 0.0.0.0 and 255.255.255.255
			var checkIPZero = 0;
			var checkIPBroad = 0;

			for (var val = 0; val < split.length; val++) {
				if (split[val] == "0") {
					checkIPZero++;
				}
				if (split[val] == "255") {
					checkIPBroad++;
				}
			}

			if (checkIPZero == 4) {
				validator.check(checkIPZero, '0.0.0.0 is not a valid IP address').is("200");
			}
			if (checkIPBroad == 4) {
				validator.check(checkIPZero, '255.255.255.255 is not a valid IP address').is("200");
			}
			break;

		case "SMTP_ADDR":
			validator.check(req.body[configItems[item]], 'SMTP Address is the wrong format').regex('^(([A-z]|[0-9])*\\.)*(([A-z]|[0-9])*)\\@((([A-z]|[0-9])*)\\.)*(([A-z]|[0-9])*)\\.(([A-z]|[0-9])*)$');
			break;
			/*
      case "SMTP_DOMAIN":
        validator.check(req.body[configItems[item]], 'SMTP Domain is the wrong format').regex('^(([A-z]|[0-9])+\\.)+(([A-z]|[0-9])+)$');
        break; 
        
      case "RECIPIENTS":
        validator.check(req.body[configItems[item]], 'Recipients list should be a comma separated list of email addresses').regex('^(([A-z]|\d)+(\\.([A-z]|\d)+)*)\\@(([A-z]|\d)+(\\.([A-z]|\d)+)+)((\\,){1}( )?(([A-z]|\d)+(\\.([A-z]|\d)+)*)\\@(([A-z]|\d)+(\\.([A-z]|\d)+)+))*$');
        break;
      */
		case "SERVICE_PREFERENCES":
			validator.check(req.body[configItems[item]], "Service Preferences string is formatted incorrectly").is('^0:[0-7](\\+|\\-)?;1:[0-7](\\+|\\-)?;2:[0-7](\\+|\\-)?;$');
			break;

		default:
			break;
		}
	}


	var errors = validator.getErrors();

	if (errors.length > 0) {
		RenderError(res, errors, "/suspects");
	} else {
		if (req.body["INTERFACE"] !== undefined && req.body["DEFAULT"] === undefined) {
			req.body["DEFAULT"] = false;
			config.UseAllInterfaces(false);
			config.WriteSetting("INTERFACE", req.body["INTERFACE"]);
		} else if (req.body["INTERFACE"] === undefined) {
			req.body["DEFAULT"] = true;
			config.UseAllInterfaces(true);
			config.WriteSetting("INTERFACE", "default");
		} else {
			config.UseAllInterfaces(req.body["DEFAULT"]);
			config.WriteSetting("INTERFACE", req.body["INTERFACE"]);
		}

		if (req.body["SMTP_USER"] !== undefined) {
			config.SetSMTPUser(req.body["SMTP_USER"]);
		}
		if (req.body["SMTP_PASS"] !== undefined) {
			config.SetSMTPPass(req.body["SMTP_PASS"]);
		}

		//if no errors, send the validated form data to the WriteSetting method
		for (var item = 4; item < configItems.length; item++) {
			if (req.body[configItems[item]] != undefined) {
				config.WriteSetting(configItems[item], req.body[configItems[item]]);
			}
		}

		var route = "/suspects";
		if (req.body['route'] != undefined) {
			route = req.body['route'];
			if (route == 'manconfig') {
				route = 'configHoneydProfiles';
			} else {
				route = 'autoConfig';
			}
		}

		res.render('saveRedirect.jade', {
			locals: {
				redirectLink: route
			}
		})
	}
});

everyone.now.ClearAllSuspects = function (callback) {
	nova.CheckConnection();
	if (!nova.ClearAllSuspects()) {
		console.log("Manually deleting CE state file:" + NovaHomePath + "/" + config.ReadSetting("CE_SAVE_FILE"));
		// If we weren't able to tell novad to clear the suspects, at least delete the CEStateFile
		try {
			fs.unlinkSync(NovaHomePath + "/" + config.ReadSetting("CE_SAVE_FILE"));
		} catch (err) {
			// this is probably because the file doesn't exist. Just ignore.
		}
	}
}

everyone.now.ClearSuspect = function (suspectIp, interface, callback) {
	nova.CheckConnection();
	var result = nova.ClearSuspect(suspectIp, interface);

	if (callback != undefined) {
		callback(result);
	}
}

everyone.now.GetInheritedEthernetList = function (parent, callback) {
	var prof = honeydConfig.GetProfile(parent);

	if (prof == null) {
		console.log("ERROR Getting profile " + parent);
		callback(null);
	} else {
		callback(prof.GetVendors(), prof.GetVendorDistributions());
	}

}

everyone.now.StartHaystack = function () {
	if (!nova.IsHaystackUp()) {
		nova.StartHaystack(false);
	}
	try {
		everyone.now.updateHaystackStatus(nova.IsHaystackUp())
	} catch (err) {};
}

everyone.now.StopHaystack = function () {
	nova.StopHaystack();
	try {
		everyone.now.updateHaystackStatus(nova.IsHaystackUp());
	} catch (err) {};
}

everyone.now.IsHaystackUp = function (callback) {
	callback(nova.IsHaystackUp());
}

everyone.now.IsNovadUp = function (callback) {
	callback(nova.IsNovadUp(false));
}

everyone.now.StartNovad = function () {
	console.log("Calling StartNovad(false)");
	var result = nova.StartNovad(false);
	console.log("Got " + result);

	console.log("Calling CheckConnection");
	var result = nova.CheckConnection();
	console.log("Done! Got " + result + ".Calling IsNovadup to check status");
	try {
		everyone.now.updateNovadStatus(nova.IsNovadUp(false));
	} catch (err) {};
}

everyone.now.StopNovad = function () {
	nova.StopNovad();
	nova.CloseNovadConnection();
	try {
		everyone.now.updateNovadStatus(nova.IsNovadUp(false));
	} catch (err) {};
}


everyone.now.sendAllSuspects = function (callback) {
	nova.sendSuspectList(callback);
}

everyone.now.sendSuspect = function (interface, ip, callback) {
	var suspect = nova.sendSuspect(interface, ip);
	if (suspect.GetIdString === undefined) {
		console.log("Failed to get suspect");
		return;
	}
	var s = new Object();
	objCopy(suspect, s);
	callback(s);
}


everyone.now.deleteUserEntry = function (usernamesToDelete, callback) {
	var username;
	for (var i = 0; i < usernamesToDelete.length; i++) {
		username = String(usernamesToDelete[i]);
		dbqCredentialsDeleteUser.run(username, function (err) {
			if (err) {
				console.log("Database error: " + err);
				callback(false);
				return;
			} else {
				callback(true);
			}
		});
	}
}

everyone.now.updateUserPassword = function (username, newPassword, callback) {
	dbqCredentialsChangePassword.run(HashPassword(newPassword), username,

	function selectCb(err, results) {
		if (err) {
			callback(false, "Unable to access user database: " + err);
			return;
		}
		callback(true);
	});
}

// Deletes a honeyd node
everyone.now.deleteNodes = function (nodeNames, callback) {
	var nodeName;
	for (var i = 0; i < nodeNames.length; i++) {
		nodeName = nodeNames[i];


		if (!honeydConfig.DeleteNode(nodeName)) {
			callback(false, "Failed to delete node " + nodeName);
			return;
		}

	}

	if (!honeydConfig.SaveAll()) {
		callback(false, "Failed to save XML templates");
		return;
	}

	callback(true, "");
}

everyone.now.deleteProfiles = function (profileNames, callback) {
	var profileName;
	for (var i = 0; i < profileNames.length; i++) {
		profileName = profileNames[i];

		if (!honeydConfig.DeleteProfile(profileName)) {
			callback(false, "Failed to delete profile " + profileName);
			return;
		}


		if (!honeydConfig.SaveAll()) {
			callback(false, "Failed to save XML templates");
			return;
		}
	}

	callback(true, "");
}

everyone.now.addWhitelistEntry = function (entry, callback) {

	// TODO: Input validation. Should be IP address or 'IP/netmask'
	// Should also be sanitized for newlines/trailing whitespace
	if (whitelistConfig.AddEntry(entry)) {
		callback(true, "");
	} else {
		callback(true, "Attempt to add whitelist entry failed");
	}
}

everyone.now.deleteWhitelistEntry = function (whitelistEntryNames, callback) {
	var whitelistEntryName;
	for (var i = 0; i < whitelistEntryNames.length; i++) {
		whitelistEntryName = whitelistEntryNames[i];

		if (!whitelistConfig.DeleteEntry(whitelistEntryName)) {
			callback(false, "Failed to delete whitelistEntry " + whitelistEntryName);
			return;
		}
	}

	callback(true, "");
}

everyone.now.GetProfile = function (profileName, callback) {
	var profile = honeydConfig.GetProfile(profileName);

	if (profile == null) {
		callback(null);
		return;
	}

	// Nowjs can't pass the object with methods, they need to be member vars
	profile.name = profile.GetName();
	profile.tcpAction = profile.GetTcpAction();
	profile.udpAction = profile.GetUdpAction();
	profile.icmpAction = profile.GetIcmpAction();
	profile.personality = profile.GetPersonality();

	profile.uptimeMin = profile.GetUptimeMin();
	profile.uptimeMax = profile.GetUptimeMax();
	profile.dropRate = profile.GetDropRate();
	profile.parentProfile = profile.GetParentProfile();

	profile.isTcpActionInherited = profile.isTcpActionInherited();
	profile.isUdpActionInherited = profile.isUdpActionInherited();
	profile.isIcmpActionInherited = profile.isIcmpActionInherited();
	profile.isPersonalityInherited = profile.isPersonalityInherited();
	profile.isEthernetInherited = profile.isEthernetInherited();
	profile.isUptimeInherited = profile.isUptimeInherited();
	profile.isDropRateInherited = profile.isDropRateInherited();

	profile.generated = profile.GetGenerated();
	profile.distribution = profile.GetDistribution();

	if (!profile.isEthernetInherited) {
		var ethVendorList = [];

		var profVendors = profile.GetVendors();
		var profDists = profile.GetVendorDistributions();

		for (var i = 0; i < profVendors.length; i++) {
			var element = {
				vendor: "",
				dist: ""
			};
			element.vendor = profVendors[i];
			element.dist = parseFloat(profDists[i]);
			ethVendorList.push(element);
		}

		profile.ethernet = ethVendorList;
	}

	callback(profile);
}

everyone.now.GetVendors = function (profileName, callback) {
	var profile = honeydConfig.GetProfile(profileName);

	if (profile == null) {
		console.log("ERROR Getting profile " + profileName);
		callback(null);
		return;
	}


	var ethVendorList = [];

	var profVendors = profile.GetVendors();
	var profDists = profile.GetVendorDistributions();

	for (var i = 0; i < profVendors.length; i++) {
		var element = {
			vendor: "",
			dist: ""
		};
		element.vendor = profVendors[i];
		element.dist = parseFloat(profDists[i]);
		ethVendorList.push(element);
	}

	callback(profVendors, profDists);
}

everyone.now.GetPorts = function (profileName, callback) {
	var ports = honeydConfig.GetPorts(profileName);

	if ((ports[0] == undefined || ports[0].portNum == "0") && profileName != "default") {
		console.log("ERROR Getting ports for profile '" + profileName + "'");
		callback(null);
		return;
	}

	for (var i = 0; i < ports.length; i++) {
		ports[i].portName = ports[i].GetPortName();
		ports[i].portNum = ports[i].GetPortNum();
		ports[i].type = ports[i].GetType();
		ports[i].behavior = ports[i].GetBehavior();
		ports[i].scriptName = ports[i].GetScriptName();
		ports[i].service = ports[i].GetService();
		ports[i].isInherited = ports[i].GetIsInherited();
	}

	callback(ports);
}


everyone.now.SaveProfile = function (profile, ports, callback, ethVendorList, addOrEdit) {
	// Check input
	var profileNameRegexp = new RegExp("[a-zA-Z]+[a-zA-Z0-9 ]*");
	var match = profileNameRegexp.exec(profile.name);
	if (match == null) {
		var err = "ERROR: Attempt to save a profile with an invalid name. Must be alphanumeric and not begin with a number.";
		callback(err);
		return;
	}

	var honeydProfile = new novaconfig.HoneydProfileBinding();

	// Move the Javascript object values to the C++ object
	honeydProfile.SetName(profile.name);
	honeydProfile.SetTcpAction(profile.tcpAction);
	honeydProfile.SetUdpAction(profile.udpAction);
	honeydProfile.SetIcmpAction(profile.icmpAction);
	honeydProfile.SetPersonality(profile.personality);

	if (ethVendorList == undefined || ethVendorList == null) {
		honeydProfile.SetEthernet(profile.ethernet);
	} else if (profile.isEthernetInherited) {
		honeydProfile.SetVendors([], []);
	} else if (profile.isEthernetInherited == false) {
		var ethVendors = [];
		var ethDists = [];

		for (var i = 0; i < ethVendorList.length; i++) {
			ethVendors.push(ethVendorList[i].vendor);
			ethDists.push(parseFloat(ethVendorList[i].dist));
		}

		honeydProfile.SetVendors(ethVendors, ethDists);
	}

	honeydProfile.SetUptimeMin(profile.uptimeMin);
	if (profile.uptimeMax != undefined) {
		honeydProfile.SetUptimeMax(profile.uptimeMax);
	} else {
		honeydProfile.SetUptimeMax(profile.uptimeMin);
	}
	honeydProfile.SetDropRate(profile.dropRate);
	honeydProfile.SetParentProfile(profile.parentProfile);

	honeydProfile.setTcpActionInherited(profile.isTcpActionInherited);
	honeydProfile.setUdpActionInherited(profile.isUdpActionInherited);
	honeydProfile.setIcmpActionInherited(profile.isIcmpActionInherited);
	honeydProfile.setPersonalityInherited(profile.isPersonalityInherited);
	honeydProfile.setEthernetInherited(profile.isEthernetInherited);
	honeydProfile.setUptimeInherited(profile.isUptimeInherited);
	honeydProfile.setDropRateInherited(profile.isDropRateInherited);
	if (profile.generated == "true") {
		honeydProfile.SetGenerated(true);
	} else {
		honeydProfile.SetGenerated(false);
	}
	honeydProfile.SetDistribution(parseFloat(profile.distribution));

	// Add new ports
	var portName;
	for (var i = 0; i < ports.size; i++) {
		// Convert the string to the proper enum number in HoneydConfiguration.h
		var behavior = ports[i].behavior;
		console.log("Port behavior is " + behavior);
		var behaviorEnumValue = new Number();
		if (behavior == "block") {
			behaviorEnumValue = 0;
		} else if (behavior == "reset") {
			behaviorEnumValue = 1;
		} else if (behavior == "open") {
			behaviorEnumValue = 2;
		} else if (behavior == "script") {
			behaviorEnumValue = 3;
		} else if (behavior == "tarpit open") {
			behaviorEnumValue = 4;
		} else if (behavior == "tarpit script") {
			behaviorEnumValue = 5;
		}

		portName = honeydConfig.AddPort(Number(ports[i].portNum), Number(ports[i].type), behaviorEnumValue, ports[i].script);

		if (portName != "") {
			honeydProfile.AddPort(portName, ports[i].isInherited);
		}
	}

	honeydConfig.SaveAll();

	if (addOrEdit == "true") {
		addOrEdit = true;
	} else {
		addOrEdit = false;
	}

	// Save the profile
	honeydProfile.Save(profile.oldName, addOrEdit);

	callback();
}

everyone.now.GetCaptureSession = function (callback) {
	var ret = config.ReadSetting("TRAINING_SESSION");
	callback(ret);
}

everyone.now.ShowAutoConfig = function (numNodesType, numNodes, interfaces, subnets, callback, route) {
	var executionString = 'haystackautoconfig';
	var nFlag = '-n';
	var rFlag = '-r';
	var iFlag = '-i';
	var aFlag = '-a';

	var hhconfigArgs = new Array();

	if (numNodesType == "fixed") {
		if (numNodes !== undefined) {
			hhconfigArgs.push(nFlag);
			hhconfigArgs.push(numNodes);
		}
	} else if (numNodesType == "ratio") {
		if (numNodes !== undefined) {
			hhconfigArgs.push(rFlag);
			hhconfigArgs.push(numNodes);
		}
	}
	if (interfaces !== undefined && interfaces.length > 0) {
		hhconfigArgs.push(iFlag);
		hhconfigArgs.push(interfaces);
	}
	if (subnets !== undefined && subnets.length > 0) {
		hhconfigArgs.push(aFlag);
		hhconfigArgs.push(subnets);
	}

	var util = require('util');
	var spawn = require('child_process').spawn;

	var autoconfig = spawn(executionString.toString(), hhconfigArgs);

	autoconfig.stdout.on('data', function (data) {
	  if(typeof callback == 'function')
	  {
		  callback('' + data);
		}
	});

	autoconfig.stderr.on('data', function (data) {
		if (/^execvp\(\)/.test(data)) {
			console.log("haystackautoconfig failed to start.");
			var response = "haystackautoconfig failed to start.";
			if(typeof route == 'function')
			{
			  route("/nodeReview", response);
			}
		}
	});

	autoconfig.on('exit', function (code) {
		console.log("autoconfig exited with code " + code);
		var response = "autoconfig exited with code " + code;
	  if(typeof route == 'function')
    {
      route("/nodeReview", response);
    }
	});
}

// TODO: Fix training
everyone.now.StartTrainingCapture = function (trainingSession, callback) {
	callback("Training mode is currently not supported");
	return;

	//config.WriteSetting("IS_TRAINING", "1");
	config.WriteSetting("TRAINING_SESSION", trainingSession.toString());

	// Check if training folder already exists
	//console.log(Object.keys(fs));
	path.exists(NovaHomePath + "/data/" + trainingSession, function (exists) {
		if (exists) {
			callback("Training session folder already exists for session name of '" + trainingSession + "'");
			return;
		} else {
			// Start the haystack
			if (!nova.IsHaystackUp()) {
				nova.StartHaystack(false);
			}

			// (Re)start NOVA
			nova.StopNovad();
			nova.StartNovad(false);

			nova.CheckConnection();

			callback();
		}
	});
}

// TODO: Fix training
everyone.now.StopTrainingCapture = function (trainingSession, callback) {
	callback("Training mode is currently not supported");
	return;
	//config.WriteSetting("IS_TRAINING", "0");
	//config.WriteSetting("TRAINING_SESSION", "null");
	nova.StopNovad();

	exec('novatrainer ' + NovaHomePath + '/data/' + trainingSession + ' ' + NovaHomePath + '/data/' + trainingSession + '/nova.dump',

	function (error, stdout, stderr) {
		callback(stderr);
	});
}

everyone.now.GetCaptureIPs = function (trainingSession, callback) {
	return trainingDb.GetCaptureIPs(NovaHomePath + "/data/" + trainingSession + "/nova.dump");
}

everyone.now.WizardHasRun = function (callback) {
	dbqFirstrunInsert.run(callback);
}


everyone.now.GetHostileEvents = function (callback) {
	dbqSuspectAlertsGet.all(

	function (err, results) {
		if (err) {
			console.log("Database error: " + err);
			// TODO implement better error handling callbacks
			callback();
			return;
		}

		callback(results);
	});
}

everyone.now.ClearHostileEvents = function (callback) {
	dbqSuspectAlertsDeleteAll.run(

	function (err) {
		if (err) {
			console.log("Database error: " + err);
			// TODO implement better error handling callbacks
			return;
		}

		callback("true");
	});
}

everyone.now.ClearHostileEvent = function (id, callback) {
	dbqSuspectAlertsDeleteAlert(id,

	function (err) {
		if (err) {
			console.log("Database error: " + err);
			// TODO implement better error handling callbacks
			return;
		}

		callback("true");
	});
}

var SendHostileEventToMothership  = function(suspect) {
  suspect.client = clientId;
  suspect.type = 'hostileSuspect';
  if(mothership != undefined)
  {
    mothership.sendUTF(JSON.stringify(suspect));
  }
};
everyone.now.SendHostileEventToMothership = SendHostileEventToMothership;

var SendBenignSuspectToMothership = function(suspect) {
  suspect.client = clientId;
  suspect.type = 'benignSuspect';
  if(mothership != undefined)
  {
    mothership.sendUTF(JSON.stringify(suspect));
  }
};
everyone.now.SendBenignSuspectToMothership = SendBenignSuspectToMothership;



everyone.now.GetLocalIP = function (interface, callback) {
	callback(nova.GetLocalIP(interface));
}

everyone.now.GenerateMACForVendor = function(vendor, callback) {
	callback(vendorToMacDb.GenerateRandomMAC(vendor));
}

everyone.now.restoreDefaultHaystackConfiguration = function(callback) {
	var source = NovaSharedPath + "/../userFiles/config/templates/*";
	var destination = NovaHomePath + "/config/templates/";
	exec('cp -f ' + source + ' ' + destination, function(err) {
		callback();
	});	
}

everyone.now.restoreDefaultSettings = function(callback) {
	var source = NovaSharedPath + "/../userFiles/config/NOVAConfig.txt";
	var destination = NovaHomePath + "/config/NOVAConfig.txt";
	exec('cp -f ' + source + ' ' + destination, function(err) {
		callback();
	});	
}

everyone.now.reverseDNS = function(ip, callback) {
	dns.reverse(ip, callback);
}

everyone.now.addTrainingPoint = function(ip, interface, features, hostility, callback) {
	if (hostility != '0' && hostility != '1') {
		callback("Error: Invalid hostility. Should be 0 or 1");
		return;
	}

	var point = features.toString() + " " + hostility + "\n";
	fs.appendFile(NovaHomePath + "/config/training/data.txt", point, function(err) {
		if (err) {
			console.log("Error: " + err);
			callback(err);
			return;
		}

		var d = new Date();
		var trainingDbString = "";
		trainingDbString += hostility + ' "User customized training point for suspect ' + ip + " added on " + d.toString() + '"\n';
		trainingDbString += "\t" + features.toString();
		trainingDbString += "\n\n";

		fs.appendFile(NovaHomePath + "/config/training/training.db", trainingDbString, function(err) {
			if (!nova.ReclassifyAllSuspects()) {
				callback("Error: Unable to reclassify suspects with new training data");
				return;
			}
			callback();
		});

	});	
}


var distributeSuspect = function (suspect) {
	var s = new Object();
	objCopy(suspect, s);
	try {
		everyone.now.OnNewSuspect(s);
	} catch (err) {};
  if(suspect.GetIsHostile() == true && parseInt(suspect.GetClassification()) != -2)
  {
    var d = new Date(suspect.GetLastPacketTime() * 1000);
    var dString = pad(d.getMonth() + 1) + "/" + pad(d.getDate()) + " " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds());
    var send = {};
    send.ip = suspect.GetIpString();
    send.classification = String(suspect.GetClassification());
    send.lastpacket = dString;
    send.ishostile = String(suspect.GetIsHostile());
    send.interface = String(suspect.GetInterface());
    
    SendHostileEventToMothership(send);
  }
  else if(suspect.GetIsHostile() == false && benignRequest && parseInt(suspect.GetClassification()) != -2)
  {
    var d = new Date(suspect.GetLastPacketTime() * 1000);
    var dString = pad(d.getMonth() + 1) + "/" + pad(d.getDate()) + " " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds());
    var send = {};
    send.ip = suspect.GetIpString();
    send.classification = String(suspect.GetClassification());
    send.lastpacket = dString;
    send.ishostile = String(suspect.GetIsHostile());
    send.interface = String(suspect.GetInterface());
    
    SendBenignSuspectToMothership(send);
  }
  else
  {

  }
};

var distributeAllSuspectsCleared = function () {
	try {
		everyone.now.AllSuspectsCleared();
	} catch (err) {
		// We can safely ignore this, it's just because no browsers are connected
	};
}

var distributeSuspectCleared = function (suspect) {
	var s = new Object;
	s['interface'] = suspect.GetInterface();
	s['ip'] = suspect.GetIpString();
	s['idString'] = suspect.GetIdString();
	everyone.now.SuspectCleared(s);
}

nova.registerOnAllSuspectsCleared(distributeAllSuspectsCleared);
nova.registerOnSuspectCleared(distributeSuspectCleared);
nova.registerOnNewSuspect(distributeSuspect);


process.on('SIGINT', function () {
	nova.Shutdown();
	process.exit();
});

function objCopy(src, dst) {
	for (var member in src) {
		if (typeof src[member] == 'function') {
			dst[member] = src[member]();
		}
		// Need to think about this
		//        else if ( typeof src[member] == 'object' )
		//        {
		//            copyOver(src[member], dst[member]);
		//        }
		else {
			dst[member] = src[member];
		}
	}
}

function switcher(err, user, success, done) {
	if (!success) {
		return done(null, false, {
			message: 'Username/password combination is incorrect'
		});
	}
	return done(null, user);
}

function pad(num) {
    return ("0" + num.toString()).slice(-2);
}

app.get('/*', passport.authenticate('basic', {session: false}));

setInterval(function () {
	try {
		everyone.now.updateHaystackStatus(nova.IsHaystackUp());
		everyone.now.updateNovadStatus(nova.IsNovadUp(false));
	} catch (err) {

	}
}, 5000);
