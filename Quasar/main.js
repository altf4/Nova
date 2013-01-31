//============================================================================
// Name        : main.js
// Copyright   : DataSoft Corporation 2011-2013
//  Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Main node.js file for the Quasar web interface of Nova
//============================================================================


// Used for debugging. Download the node-segfault-handler to use
//var segvhandler = require('./node_modules/segvcatcher/lib/segvhandler')
//segvhandler.registerHandler();

//var agent = require('webkit-devtools-agent');

/*var memwatch = require('memwatch');

memwatch.on('leak', function(info)
{
    console.log(info);
});

var hd = new memwatch.HeapDiff();
memwatch.on('stats', function(stats)
{
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

// Modules from NodejsModule/Javascript
var LOG = require("../NodejsModule/Javascript/Logger").LOG;

if (!honeydConfig.LoadAllTemplates())
{
    LOG("ERROR", "Call to initial LoadAllTemplates failed!");
}

var fs = require('fs');
var path = require('path');
var jade = require('jade');
var express = require('express');
var util = require('util');
var passport = require('passport');
var BasicStrategy = require('passport-http').BasicStrategy;
var sql = require('sqlite3').verbose();
var validCheck = require('validator').check;
var sanitizeCheck = require('validator').sanitize;
var crypto = require('crypto');
var exec = require('child_process').exec;
var nowjs = require("now");
var Validator = require('validator').Validator;
var dns = require('dns');
var wrench = require('wrench');

var classifiersConstructor = new require('./classifiers.js');
var classifiers = new classifiersConstructor(config);

var Tail = require('tail').Tail;
var NovaHomePath = config.GetPathHome();
var NovaSharedPath = config.GetPathShared();
var novadLogPath = "/var/log/nova/Nova.log";
var novadLog = new Tail(novadLogPath);

var honeydLogPath = "/var/log/nova/Honeyd.log";
var honeydLog = new Tail(honeydLogPath);

var benignRequest = false;
var pulsar;

var autoconfig;

var interfaceAliases = new Object();
ReloadInterfaceAliasFile();

var RenderError = function (res, err, link)
{
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

var HashPassword = function (password, salt)
{
    var shasum = crypto.createHash('sha1');
    shasum.update(password + salt);
    return shasum.digest('hex');
};

LOG("ALERT", "Starting QUASAR version " + config.GetVersionString());


process.chdir(NovaHomePath);

var DATABASE_HOST = config.ReadSetting("DATABASE_HOST");
var DATABASE_USER = config.ReadSetting("DATABASE_USER");
var DATABASE_PASS = config.ReadSetting("DATABASE_PASS");

var databaseOpenResult = function (err)
{
    if(err === null)
    {
        console.log("Opened sqlite3 database file.");
    }
    else
    {
        LOG("ERROR", "Error opening sqlite3 database file: " + err);
    }
}

var novaDb = new sql.Database(NovaHomePath + "/data/novadDatabase.db", sql.OPEN_READWRITE, databaseOpenResult);
var db = new sql.Database(NovaHomePath + "/data/quasarDatabase.db", sql.OPEN_READWRITE, databaseOpenResult);

// Prepare query statements
var dbqCredentialsRowCount = db.prepare('SELECT COUNT(*) AS rows from credentials');
var dbqCredentialsCheckLogin = db.prepare('SELECT user, pass FROM credentials WHERE user = ? AND pass = ?');
var dbqCredentialsGetUsers = db.prepare('SELECT user FROM credentials');
var dbqCredentialsGetUser = db.prepare('SELECT user FROM credentials WHERE user = ?');
var dbqCredentialsGetSalt = db.prepare('SELECT salt FROM credentials WHERE user = ?');
var dbqCredentialsChangePassword = db.prepare('UPDATE credentials SET pass = ?, salt = ? WHERE user = ?');
var dbqCredentialsInsertUser = db.prepare('INSERT INTO credentials VALUES(?, ?, ?)');
var dbqCredentialsDeleteUser = db.prepare('DELETE FROM credentials WHERE user = ?');

var dbqFirstrunCount = db.prepare("SELECT COUNT(*) AS rows from firstrun");
var dbqFirstrunInsert = db.prepare("INSERT INTO firstrun values(datetime('now'))");

var dbqSuspectAlertsGet = novaDb.prepare('SELECT suspect_alerts.id, timestamp, suspect, classification, ip_traffic_distribution,port_traffic_distribution,packet_size_mean,packet_size_deviation,distinct_ips,distinct_tcp_ports,distinct_udp_ports,avg_tcp_ports_per_host,avg_udp_ports_per_host,tcp_percent_syn,tcp_percent_fin,tcp_percent_rst,tcp_percent_synack,haystack_percent_contacted FROM suspect_alerts LEFT JOIN statistics ON statistics.id = suspect_alerts.statistics');
var dbqSuspectAlertsDeleteAll = novaDb.prepare('DELETE FROM suspect_alerts');
var dbqSuspectAlertsDeleteAlert = novaDb.prepare('DELETE FROM suspect_alerts where id = ?');

passport.serializeUser(function (user, done)
{
    done(null, user);
});

passport.deserializeUser(function (user, done)
{
    done(null, user);
});

passport.use(new BasicStrategy(

function (username, password, done)
{
    var user = username;
    process.nextTick(function ()
    {

        dbqCredentialsRowCount.all(function (err, rowcount)
        {
            if(err)
            {
                console.log("Database error: " + err);
            }

            // If there are no users, add default nova and log in
            if(rowcount[0].rows === 0)
            {
                console.log("No users in user database. Creating default user.");
                dbqCredentialsInsertUser.run('nova', HashPassword('toor', 'root'), 'root', function (err)
                {
                    if(err)
                    {
                        throw err;
                    }

                    switcher(err, user, true, done);
                });
            }
            else
            {
              dbqCredentialsGetSalt.all(user, function cb(err, salt)
              {
                if(err || (salt[0] == undefined))
                {
                  switcher(err, user, false, done);
                }
                else
                {
                    dbqCredentialsCheckLogin.all(user, HashPassword(password, salt[0].salt),
                    function selectCb(err, results)
                    {
                        if(err)
                        {
                            console.log("Database error: " + err);
                        }
                        if(results[0] === undefined)
                        {
                            switcher(err, user, false, done);
                        } 
                        else if(user === results[0].user)
                        {
                            switcher(err, user, true, done);
                        } 
                        else
                        {
                            switcher(err, user, false, done);
                        }
              });
            }});
          }
        });
    });
}));

// Setup TLS
var app;
if (config.ReadSetting("QUASAR_WEBUI_TLS_ENABLED") == "1")
{
    var keyPath = config.ReadSetting("QUASAR_WEBUI_TLS_KEY");
    var certPath = config.ReadSetting("QUASAR_WEBUI_TLS_CERT");
    var passPhrase = config.ReadSetting("QUASAR_WEBUI_TLS_PASSPHRASE");
    express_options = {
        key: fs.readFileSync(NovaHomePath + keyPath),
        cert: fs.readFileSync(NovaHomePath + certPath),
        passphrase: passPhrase
    };
 
    app = express.createServer(express_options);
} else {
    app = express.createServer();
}

app.configure(function ()
{
    app.use(passport.initialize());
    app.use(express.bodyParser());
	app.use(passport.authenticate('basic', {session: false}));
    app.use(app.router);
    app.use(express.static(NovaSharedPath + '/Quasar/www'));
});

app.set('views', __dirname + '/views');

app.set('view options', {
    layout: false
});

// Do the following for logging
console.info("Logging to ./serverLog.log");
var logFile = fs.createWriteStream('./serverLog.log', {flags: 'a'});
app.use(express.logger({stream: logFile}));

var WEB_UI_PORT = config.ReadSetting("WEB_UI_PORT");
console.info("Listening on port " + WEB_UI_PORT);
app.listen(WEB_UI_PORT);
var everyone = nowjs.initialize(app);

var initLogWatch = function ()
{
    var novadLog = new Tail(novadLogPath);
    novadLog.on("line", function (data)
    {
        try {
            everyone.now.newLogLine(data);
        } catch (err)
        {

        }
    });

    novadLog.on("error", function (data)
    {
        LOG("ERROR", "Novad log watch error: " + data);
        try {
            everyone.now.newLogLine(data)
        } catch (err)
        {
            LOG("ERROR", "Novad log watch error: " + err);
        }
    });


    var honeydLog = new Tail(honeydLogPath);
    honeydLog.on("line", function (data)
    {
        try {
            everyone.now.newHoneydLogLine(data);
        } catch (err)
        {

        }
    });

    honeydLog.on("error", function (data)
    {
        LOG("ERROR", "Honeyd log watch error: " + data);
        try {
            everyone.now.newHoneydLogLine(data)
        } catch (err)
        {
            LOG("ERROR", "Honeyd log watch error: " + err);

        }
    });
}

initLogWatch();

if(config.ReadSetting('MASTER_UI_ENABLED') === '1')
{
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // SOCKET STUFF FOR PULSAR 
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Thoughts: Probably going to need a global variable for the websocket connection to
  // Pulsar; an emit needs to be made to Pulsar in certain circumstances,
  // such as recieving a hostile suspect, so having that be accessible to the nowjs methods
  // is ideal. Also need to find a way to maintain IP address of Pulsar and any needed
  // credentials for reboots, etc. 
  
  // The reason for the + 1 at the end is because that's the port in use for the 
  // server-to-server component of wbsockets on Pulsar. Have to do something in case 
  // the port gets changed on the Pulsar machine
  var connected = config.ReadSetting('MASTER_UI_IP') + ':' + (parseInt(config.ReadSetting('MASTER_UI_PORT')) + 1);
  
  var WebSocketClient = require('websocket').client;
  var client;
  
  if (config.ReadSetting("QUASAR_TETHER_TLS_ENABLED"))
  {
    client = new WebSocketClient({
      tlsOptions: {
        cert: fs.readFileSync(NovaHomePath + config.ReadSetting("QUASAR_TETHER_TLS_CERT")),
        key: fs.readFileSync(NovaHomePath + config.ReadSetting("QUASAR_TETHER_TLS_KEY")),
        passphrase: config.ReadSetting("QUASAR_TETHER_TLS_PASSPHRASE")
      }
    });
  } else {
    client = new WebSocketClient();
  }
  // TODO: Make configurable
  var clientId = config.ReadSetting('MASTER_UI_CLIENT_ID');
  var reconnecting = false;
  var clearReconnect;
  var reconnectTimer = parseInt(config.ReadSetting('MASTER_UI_RECONNECT_TIME')) * 1000;
  
  // If the connection fails, print an error message
  client.on('connectFailed', function(error)
  {
    console.log('Connect to Pulsar error: ' + error.toString());
    if(!reconnecting)
    {
      console.log('No current attempts to reconnect, starting reconnect attempts every ', (reconnectTimer / 1000) ,' seconds.');
      // TODO: Don't have static lengths for reconnect interval; make configurable
      clearReconnect = setInterval(function(){console.log('attempting reconnect to wss://' + connected); client.connect('wss://' + connected, null);}, reconnectTimer);
      reconnecting = true;
    }
  });
  
  // If we successfully connect to pulsar, perform event-based actions here.
  // The interface for websockets is a little less nice than socket.io, simply because
  // you cannot name your own events, everything has to be done within the message
  // directive; it's only one more level of indirection, however, so no big deal
  client.on('connect', function(connection){
    // When the connection is established, we save the connection variable
    // to p global so that actions can be taken on the connection
    // outside of this callback if needed (hostile suspect, etc.)
    reconnecting = false;
    clearInterval(clearReconnect);
    pulsar = connection;
    var quick = {};
    quick.type = 'addId';
    quick.id = clientId;
    quick.nova = nova.IsNovadUp(false).toString();
    quick.haystack = nova.IsHaystackUp(false).toString();
    quick.benignRequest = (benignRequest == true ? 'true' : 'false');
    // I don't know that we HAVE to use UTF8 here, there's a send() method as 
    // well as a 'data' member inside the message objects instead of utf8Data.
    // But, as it was in the Websockets tutorial Pherric found, we'll use it for now
    pulsar.sendUTF(JSON.stringify(quick));
    console.log('Registering client Id with pulsar');
    
    var configSend = {};
    configSend.type = 'registerConfig';
    configSend.id = clientId;
    configSend.filename = 'NOVAConfig@' + clientId + '.txt';
    configSend.file = fs.readFileSync(NovaHomePath + '/config/NOVAConfig.txt', 'utf8');
    pulsar.sendUTF(JSON.stringify(configSend));
    console.log('Registering NOVAConfig with pulsar');
  
    var interfaceSend = {};
    interfaceSend.type = 'registerClientInterfaces';
    interfaceSend.id = clientId;
    interfaceSend.filename = 'iflist@' + clientId + '.txt';
    interfaceSend.file = config.ListInterfaces().sort().join();
    pulsar.sendUTF(JSON.stringify(interfaceSend));
    console.log('Registering interfaces with pulsar');
  
    console.log('successfully connected to pulsar');
  
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
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'stopNovad':
              nova.StopNovad();
              nova.CloseNovadConnection();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Novad is being stopped on ' + clientId;
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'startHaystack':
              if(!nova.IsHaystackUp())
              {
                nova.StartHaystack(false);
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Haystack is being started on ' + clientId;
                pulsar.sendUTF(JSON.stringify(response));
              }
              else
              {
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Haystack is already up, doing nothing';
                pulsar.sendUTF(JSON.stringify(response));
              }
              break;
            case 'stopHaystack':
              nova.StopHaystack();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Haystack is being stopped on ' + clientId;
              pulsar.sendUTF(JSON.stringify(response));
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
              pulsar.sendUTF(JSON.stringify(response));
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
              pulsar.sendUTF(JSON.stringify(response));
              configSend.file = fs.readFileSync(NovaHomePath + '/config/NOVAConfig.txt', 'utf8');
              pulsar.sendUTF(JSON.stringify(configSend));
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
              if(json_args.interface !== undefined && json_args.interface.length > 0)
              {
                hhconfigArgs.push(iFlag);
                hhconfigArgs.push(json_args.interface);
              }
            
              var util = require('util');
              var spawn = require('child_process').spawn;
            
              autoconfig = spawn(executionString.toString(), hhconfigArgs);
            
              autoconfig.stdout.on('data', function (data){
                console.log('' + data);
              });
            
              autoconfig.stderr.on('data', function (data){
                if (/^execvp\(\)/.test(data))
                {
                  console.log("haystackautoconfig failed to start.");
                  var message = "haystackautoconfig failed to start.";
                  var response = {};
                  response.id = clientId;
                  response.type = 'response';
                  response.response_message = message;
                  pulsar.sendUTF(JSON.stringify(response));
                }
              });
            
              autoconfig.on('exit', function (code){
                console.log("autoconfig exited with code " + code);
                var message = "autoconfig exited with code " + code;
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = message;
                pulsar.sendUTF(JSON.stringify(response));
              });

              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Haystack Autoconfiguration commencing';
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'suspectDetails':
              var suspectString = nova.GetSuspectDetailsString(json_args.ip, json_args.iface);
              suspectString = suspectString.replace(/(\r\n|\r|\n)/gm, "<br>");
              var response = {};
              response.id = clientId;
              response.type = 'detailsReceived';
              response.data = suspectString;
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'clearSuspects':
              everyone.now.ClearAllSuspects(function(){
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Suspects cleared on ' + json_args.id;
                pulsar.sendUTF(JSON.stringify(response));
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
      // the stored IP of Pulsar to make the reconnect attempts
      pulsar = undefined;
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
  
  setInterval(function()
  {
    if(pulsar != undefined && pulsar != null)
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
      pulsar.sendUTF(JSON.stringify(message1));
      pulsar.sendUTF(JSON.stringify(message2));
    }
  }, 5000);
  //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

function cNodeToJs(node)
{
	var ret = {};
    ret.enabled = node.IsEnabled();
    ret.pfile = node.GetProfile();
    ret.ip = node.GetIP();
    ret.mac = node.GetMAC();
    ret.interface = node.GetInterface();
	return ret;
}

app.get('/honeydConfigManage', function(req, res){
  var tab;
  if (req.query["tab"] === undefined)
  {
    tab = "settingsGroups";
  }
  else
  {
    tab = req.query["tab"];
  }


  var nodeNames = honeydConfig.GetNodeMACs();
  var nodeList = [];
  
  for (var i = 0; i < nodeNames.length; i++)
  {
      var node = honeydConfig.GetNode(nodeNames[i]);
      var push = cNodeToJs(node);
      nodeList.push(push);
  }

  var interfaces = config.ListInterfaces().sort();

  res.render('honeydConfigManage.jade', {
    locals: {
      configurations: honeydConfig.GetConfigurationsList(),
      current: config.GetCurrentConfig(),
      nodes: nodeList,
      INTERFACES: interfaces,
      interfaceAliases: ConvertInterfacesToAliases(interfaces),
      tab: tab
    }
  });
});

app.get('/downloadNovadLog.log', function (req, res)
{
    res.download(novadLogPath, 'novadLog.log');
});

app.get('/downloadHoneydLog.log', function (req, res)
{
    res.download(honeydLogPath, 'honeydLog.log');
});

app.get('/nodeState.csv', function(req, res)
{
  var nodeNames = honeydConfig.GetNodeMACs();
  var nodeList = [];

  var csvString = "ENABLED,IP,INTERFACE,MAC,PROFILE\n";
  
  for (var i = 0; i < nodeNames.length; i++)
  {
      var node = honeydConfig.GetNode(nodeNames[i]);
      csvString += node.IsEnabled() + ",";
      csvString += node.GetIP() + ",";
      csvString += node.GetInterface() + ",";
      csvString += node.GetMAC() + ",";
      csvString += node.GetProfile();
      csvString += "\n";
  }

  res.header('Content-Type', 'text/csv');
  res.send(csvString);
});

app.get('/novaState.csv', function (req, res)
{
    exec('novacli get all csv > ' + NovaHomePath + "/state.csv",
    function(error, stdout, stderr)
    {
        if (error != null)
        {
            // Don't really care. Probably failed because novad was down.
            //console.log("exec error: " + error);
        }
        
        fs.readFile(NovaHomePath + "/state.csv", 'utf8', function(err, data)
        {
            res.header('Content-Type', 'text/csv');
            var reply = data.toString();
            res.send(reply);
        });
    });
});

app.get('/viewNovadLog', function (req, res)
{
    fs.readFile(novadLogPath, 'utf8', function (err, data)
    {
        if (err)
        {
            RenderError(res, "Unable to open NOVA log file for reading due to error: " + err);
            return;
        } else {
            res.render('viewNovadLog.jade', {
                locals: {
                    log: data
                }
            });
        }
    });
});

app.get('/viewHoneydLog', function (req, res)
{
    fs.readFile(honeydLogPath, 'utf8', function (err, data)
    {
        if (err)
        {
            RenderError(res, "Unable to open honeyd log file for reading due to error: " + err);
            return;
        } else {
            res.render('viewHoneydLog.jade', {
                locals: {
                    log: data
                }
            });
        }
    });
});

app.get('/advancedOptions', function (req, res)
{
    var all = config.ListInterfaces().sort();
    var used = config.GetInterfaces().sort();

    var pass = [];

    for (var i in all)
    {
        var checked = false;

        for (var j in used)
        {
            if (all[i] === used[j])
            {
                checked = true;
                break;
            }
        }

        if (checked)
        {
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
			, TRAINING_DATA_PATH: config.ReadSetting("TRAINING_DATA_PATH")
      , RSYSLOG_IP: getRsyslogIp()
            , supportedEngines: nova.GetSupportedEngines()
        }
    });
});

function renderBasicOptions(jadefile, res, req)
{
    var all = config.ListInterfaces().sort();
    var used = config.GetInterfaces().sort();

    var pass = [];

    for (var i in all)
    {
        var checked = false;

        for (var j in used)
        {
            if (all[i] === used[j])
            {
                checked = true;
                break;
            }
        }

        if (checked)
        {
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

    for (var i in all)
    {
        var checked = false;

        for (var j in used)
        {
            if (all[i] === used[j])
            {
                checked = true;
                break;
            } else if (used[j].length == 1 && used.length > 1)
            {
                if (all[i] === used)
                {
                    checked = true;
                    break;
                }
            }
        }

        if (checked)
        {
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

    var ifaceForConversion = new Array();
    for (var i = 0; i < pass.length; i++)
    {
        ifaceForConversion.push(pass[i].iface);
    }
    res.render(jadefile, {
        locals: {
            INTERFACES: pass,
            INTERFACE_ALIASES: ConvertInterfacesToAliases(ifaceForConversion),
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
            EMAIL_ALERTS_ENABLED: config.ReadSetting("EMAIL_ALERTS_ENABLED"),
            SERVICE_PREFERENCES: config.ReadSetting("SERVICE_PREFERENCES"),
            RECIPIENTS: config.ReadSetting("RECIPIENTS")
        }
    });
}

app.get('/error', function (req, res)
{
    RenderError(res, req.query["errorDetails"], req.query["redirectLink"]);
    return;
});

app.get('/basicOptions', function (req, res)
{
    renderBasicOptions('basicOptions.jade', res, req);
});

app.get('/configHoneydNodes', function (req, res)
{
    if (!honeydConfig.LoadAllTemplates())
    {
        RenderError(res, "Unable to load honeyd configuration XML files");
        return;
    }

    var profiles = honeydConfig.GetLeafProfileNames();
    
    var interfaces = config.ListInterfaces().sort();
    
  res.render('configHoneydNodes.jade', {
    locals: {
      INTERFACES: interfaces,
      interfaceAliases: ConvertInterfacesToAliases(interfaces),
      profiles: profiles,
      currentGroup: config.GetGroup()
    }
  });
});

app.get('/GetSuspectDetails', function (req, res)
{
    if (req.query["ip"] === undefined)
    {
        RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/");
        return;
    }
    
    if (req.query["interface"] === undefined)
    {
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

app.get('/editHoneydNode', function (req, res)
{
    if (req.query["node"] === undefined)
    {
        RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/configHoneydNodes");
        return;
    }
    
    var nodeName = req.query["node"];
    var node = honeydConfig.GetNode(nodeName);

    if (node == null)
    {
        RenderError(res, "Unable to fetch node: " + nodeName, "/configHoneydNodes");
        return;
    }

    var interfaces;
    if (nodeName == "doppelganger")
    {
        interfaces = config.ListLoopbacks().sort();
    } else {
        interfaces = config.ListInterfaces().sort();
    }

    res.render('editHoneydNode.jade', {
        locals: {
            oldName: nodeName,
            INTERFACES: interfaces,
            interfaceAliases: ConvertInterfacesToAliases(interfaces),
            profiles: honeydConfig.GetProfileNames(),
            profile: node.GetProfile(),
            interface: node.GetInterface(),
            ip: node.GetIP(),
            mac: node.GetMAC(),
            portSet: node.GetPortSet()
        }
    })
});

app.get('/editHoneydProfile', function (req, res)
{
    if (req.query["profile"] === undefined)
    {
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

app.get('/addHoneydProfile', function (req, res)
{
    if (req.query["parent"] === undefined)
    {
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

app.get('/customizeTraining', function (req, res)
{
    trainingDb = new novaconfig.CustomizeTrainingBinding();
    res.render('customizeTraining.jade', {
        locals: {
            desc: trainingDb.GetDescriptions(),
            uids: trainingDb.GetUIDs(),
            hostiles: trainingDb.GetHostile()
        }
    })
});

app.get('/importCapture', function (req, res)
{
    if (req.query["trainingSession"] === undefined)
    {
        RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.");
        return;
    }

    var trainingSession = req.query["trainingSession"];
    trainingSession = NovaHomePath + "/data/" + trainingSession + "/nova.dump";
    var ips = trainingDb.GetCaptureIPs(trainingSession);

    if (ips === undefined)
    {
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

everyone.now.changeGroup = function(group, callback)
{
    callback(config.SetGroup(group));
}

everyone.now.ChangeNodeInterfaces = function(nodes, newIntf, cb)
{
  honeydConfig.ChangeNodeInterfaces(nodes, newIntf);
  if(typeof cb == 'function')
  {
    cb();
  }
}

everyone.now.GetProfileNames = function(callback)
{
  callback(honeydConfig.GetProfileNames());
}

everyone.now.GetLeafProfileNames = function(callback)
{
  callback(honeydConfig.GetLeafProfileNames());
}

app.post('/importCaptureSave', function (req, res)
{
    var hostileSuspects = new Array();
    var includedSuspects = new Array();
    var descriptions = new Object();

    var trainingSession = req.query["trainingSession"];
    trainingSession = NovaHomePath + "/data/" + trainingSession + "/nova.dump";

    var trainingDump = new novaconfig.TrainingDumpBinding();
    if (!trainingDump.LoadCaptureFile(trainingSession))
    {
        ReadSetting(res, "Unable to parse dump file: " + trainingSession);
    }

    trainingDump.SetAllIsIncluded(false);
    trainingDump.SetAllIsHostile(false);

    for (var id in req.body)
    {
        id = id.toString();
        var type = id.split('_')[0];
        var ip = id.split('_')[1];

        if (type == 'include')
        {
            includedSuspects.push(ip);
            trainingDump.SetIsIncluded(ip, true);
        } else if (type == 'hostile')
        {
            hostileSuspects.push(ip);
            trainingDump.SetIsHostile(ip, true);
        } else if (type == 'description')
        {
            descriptions[ip] = req.body[id];
            trainingDump.SetDescription(ip, req.body[id]);
        } else {
            console.log("ERROR: Got invalid POST values for importCaptureSave");
        }
    }

    // TODO: Don't hard code this path
    if (!trainingDump.SaveToDb(NovaHomePath + "/config/training/training.db"))
    {
        RenderError(res, "Unable to save to training db");
        return;
    }

    res.render('saveRedirect.jade', {
        locals: {
            redirectLink: "/customizeTraining"
        }
    })

});

app.get('/configWhitelist', function (req, res)
{
    res.render('configWhitelist.jade', {
        locals: {
            whitelistedIps: whitelistConfig.GetIps(),
            whitelistedRanges: whitelistConfig.GetIpRanges(),
            INTERFACES: config.ListInterfaces().sort()
        }
    })
});

app.get('/editUsers', function (req, res)
{
    var usernames = new Array();
    dbqCredentialsGetUsers.all(

    function (err, results)
    {
        if (err)
        {
            RenderError(res, "Database Error: " + err);
            return;
        }

        var usernames = new Array();
        for (var i in results)
        {
            usernames.push(results[i].user);
        }
        res.render('editUsers.jade', {
            locals: {
                usernames: usernames
            }
        });
    });
});

app.get('/configWhitelist', function (req, res)
{
    res.render('configWhitelist.jade', {
        locals: {
            whitelistedIps: whitelistConfig.GetIps(),
            whitelistedRanges: whitelistConfig.GetIpRanges()
        }
    })
});

app.get('/suspects', function (req, res)
{
    res.render('main.jade', {
        user: req.user,
        featureNames: nova.GetFeatureNames(),
    });
});

app.get('/monitor', function (req, res)
{
    var suspectIp = req.query["ip"];
    var suspectInterface = req.query["interface"];
    
    res.render('monitor.jade', {
        featureNames: nova.GetFeatureNames()
        , suspectIp: suspectIp
        , suspectInterface: suspectInterface
    });
});

app.get('/events', function (req, res)
{
    res.render('events.jade', {
        featureNames: nova.GetFeatureNames()
    });
});

app.get('/', function (req, res)
{
    dbqFirstrunCount.all(

    function (err, results)
    {
        if (err)
        {
            RenderError(res, "Database Error: " + err);
            return;
        }

        if (results[0].rows == 0)
        {
            res.redirect('/welcome');
        } else {
            res.redirect('/suspects');
        }

    });
});

app.get('/createNewUser', function (req, res)
{
    res.render('createNewUser.jade');
});

app.get('/welcome', function (req, res)
{
    res.render('welcome.jade');
});

app.get('/setup1', function (req, res)
{
    res.render('setup1.jade');
});

app.get('/setup2', function (req, res)
{
    renderBasicOptions('setup2.jade', res, req)
});

app.get('/setup3', function (req, res)
{
    res.render('hhautoconfig.jade', {
        user: req.user,
        INTERFACES: config.ListInterfaces().sort(),
        SCANERROR: ""
    });
});

// Training data capture via Quasar isn't currently supported
//app.get('/CaptureTrainingData', function (req, res)
//{
//  res.render('captureTrainingData.jade');
//});
app.get('/about', function (req, res)
{
    res.render('about.jade');
});

app.post('/createNewUser', function (req, res)
{
    var password = req.body["password"];
    var userName = req.body["username"];
	
	if (password.length == 0 || userName.length == 0) {
      RenderError(res, "Can not have blank username or password!", "/setup1");
      return;
	}

    dbqCredentialsGetUser.all(userName,

    function selectCb(err, results, fields)
    {
        if (err)
        {
            RenderError(res, "Database Error: " + err, "/createNewUser");
            return;
        }

        if (results[0] == undefined)
        {
          var salt = '';
          var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
          for(var i = 0; i < 8; i++)
          {
            salt += possible[Math.floor(Math.random() * possible.length)];
          }
            dbqCredentialsInsertUser.run(userName, HashPassword(password, salt), salt, function ()
            {
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

app.post('/createInitialUser', function (req, res)
{
    var password = req.body["password"];
    var userName = req.body["username"];

	if (password.length == 0 || userName.length == 0) {
      RenderError(res, "Can not have blank username or password!", "/setup1");
      return;
	}

    dbqCredentialsGetUser.all(userName,

    function selectCb(err, results)
    {
        if (err)
        {
            RenderError(res, "Database Error: " + err);
            return;
        }

        if (results[0] == undefined)
        {
          var salt = '';
      var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
      for(var i = 0; i < 8; i++)
      {
        salt += possible[Math.floor(Math.random() * possible.length)];
      }
            dbqCredentialsInsertUser.run(userName, HashPassword(password, salt), salt);
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

app.get('/autoConfig', function (req, res)
{
	var interfaces = config.ListInterfaces().sort();
    res.render('hhautoconfig.jade', {
        user: req.user,
        INTERFACES: interfaces,
        interfaceAliases: ConvertInterfacesToAliases(interfaces),
        SCANERROR: "",
        GROUPS: honeydConfig.GetConfigurationsList()
    });
});

app.get("/editTLSCerts", function (req, res)
{
    res.render('editTLSCerts.jade');    
});

app.get("/editClassifiers", function (req, res)
{
	res.render('editClassifiers.jade', {
		locals: {
        	classifiers: classifiers.getClassifiers()
		}
	});	
});

app.get("/editClassifier", function (req, res)
{
	var featureNames = nova.GetFeatureNames();
    if(req.query['classifierIndex'] == undefined)
    {
		var classifier = {
			type: "THRESHOLD_TRIGGER"
			, mode: "HOSTILE_OVERRIDE"
			, config: "/config/newClassifier.config"
			, weight: "0"
			, strings: {}
			, index: '-1'
		};
        
		var features = [];
        for (var i = 0; i < featureNames.length; i++) {
          var feature = {
            name: featureNames[i]
            , enabled: true
            , weight: 1
            , threshold: "-"
          };
  
          features.push(feature);
        }

		classifier.features = features;
  	} else {
		var index = req.query['classifierIndex'];
		var classifier = classifiers.getClassifier(index);
     
		var enabledFeatures = String(classifier.strings["ENABLED_FEATURES"]);
		var weightString = classifier.strings["FEATURE_WEIGHTS"];
		var thresholdString = classifier.strings["THRESHOLD_HOSTILE_TRIGGERS"];
		var features = [];
        for (var i = 0; i < featureNames.length; i++)
		{
        	var feature = {
            	name: featureNames[i]
            	, enabled: enabledFeatures.charAt(i) == '1'
				, weight: 1
				, threshold: "-"
		  	}
           
		  	if (classifier.type == "KNN")
		  	{
		 		feature.weight = weightString.split(" ")[i];	 
		  	} 
		  	else if (classifier.type == "THRESHOLD_TRIGGER")
		  	{
		 		feature.threshold = thresholdString.split(" ")[i];
		  	}
		  
            features.push(feature);
        }
		
		classifier.index = index;
	}
  
	classifier.features = features;


	res.render('editClassifier.jade', {
		locals: {
        	classifier: classifier
			, featureNames: nova.GetFeatureNames()
		}
	});	
});

everyone.now.deleteClassifier = function(index, callback)
{
	classifiers.deleteClassifier(index);
	if (callback) callback();
}

everyone.now.saveClassifier = function(classifier, index, callback)
{
	// Convert the model instance settings to strings for config file
	var enabledFeaturesString = "";
	var weightString = "";
	var thresholdString = "";

	for (var i = 0; i < classifier.features.length; i++)
	{
		if (classifier.features[i].enabled)
		{
			enabledFeaturesString += "1";
		}
		else
		{
			enabledFeaturesString += "0";
		}

		if (classifier.type == "KNN")
		{
			weightString += String(classifier.features[i].weight) + " ";
		}
		else if (classifier.type == "THRESHOLD_TRIGGER")
		{
			thresholdString += classifier.features[i].threshold + " ";
		}
	}
	
	classifier.strings = {};
	classifier.strings["ENABLED_FEATURES"] = enabledFeaturesString;
	if (classifier.type == "KNN")
	{
		classifier.strings["FEATURE_WEIGHTS"] = weightString;
	}
	else if (classifier.type == "THRESHOLD_TRIGGER")
	{
		classifier.strings["THRESHOLD_HOSTILE_TRIGGERS"] = thresholdString;
	}

	classifiers.saveClassifier(classifier, index);
	if (callback) callback();
}

app.get("/interfaceAliases", function (req, res)
{
    ReloadInterfaceAliasFile();
    res.render('interfaceAliases.jade', {
        locals: {
            interfaceAliases: interfaceAliases
            , INTERFACES: config.ListInterfaces().sort(),
        }
    });
});

app.post("/editTLSCerts", function (req, res)
{
    if (req.files["cert"] == undefined || req.files["key"] == undefined)
    {
        RenderError(res, "Invalid form submission. This was likely caused by refreshing a page you shouldn't.");
        return;
    }

    if (req.files["cert"].size == 0 || req.files["key"].size == 0)
    {
        RenderError(res, "You must choose both a key and certificate to upload");
        return;
    }

    fs.readFile(req.files["key"].path, function (readErrKey, data)
    {
        fs.writeFile(NovaHomePath + "/config/keys/quasarKey.pem", data, function(writeErrKey)
        {
            
            fs.readFile(req.files["cert"].path, function (readErrCert, certData)
            {
                fs.writeFile(NovaHomePath + "/config/keys/quasarCert.pem", certData, function(writeErrCert)
                {
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

app.post('/scripts', function(req, res){
  if(req.files['script'] == undefined || req.body['name'] == undefined)
  {
    RenderError(res, "Invalid form submission. This was likely caused by refreshing a page you shouldn't.");
    return;
  }
  if(req.files['script'] == '' || req.body['name'] == '')
  {
    RenderError(res, "Must select both a file and a name for the script to add");
    return;
  }
  
  function clean(string)
  {
    var temp = '';
    for(var i in string)
    {
      if(/[a-zA-Z0-9\.]/.test(string[i]) == true)
      {
        temp += string[i].toLowerCase();
      }
    }
    
    return temp;
  }
    
  var filename = req.files['script'].name;
  
  filename = 'userscript_' + clean(filename);
  
  // should create a 'user' folder within the honeyd script
  // path for user uploaded scripts, maybe add a way for the
  // the user to choose where the script goes within the current
  // file structure later.
  var pathToSave = '/usr/share/honeyd/scripts/misc/' + filename;
  
  fs.readFile(req.files['script'].path, function(readErr, data){
    if(readErr != null)
    {
      RenderError(res, 'Failed to read designated script file');
      return;
    }
    
    fs.writeFileSync(pathToSave, data);
    
    fs.chmodSync(pathToSave, '775');
    
    pathToSave = req.body['shell'] + ' ' + pathToSave + ' ' + req.body['args'];
  
    honeydConfig.AddScript(req.body['name'], pathToSave);
  
    honeydConfig.SaveAllTemplates();
  });
  
  res.render('saveRedirect.jade', {
    locals: {
      redirectLink: '/scripts'
    }
  });
});

app.post('/honeydConfigManage', function (req, res){
  var newName = (req.body['newName'] != undefined ? req.body['newName'] : req.body['newNameClone']);
  var configToClone = (req.body['cloneSelect'] != undefined ? req.body['cloneSelect'] : '');
  var cloneBool = false;
  if(configToClone != '')
  {
    cloneBool = true;
  }
  
  if((new RegExp('^[a-zA-Z0-9 -_]+$')).test(newName))
  {
    honeydConfig.AddConfiguration(newName, cloneBool, configToClone);
    honeydConfig.SwitchConfiguration(newName);
    honeydConfig.LoadAllTemplates();
  
    res.render('saveRedirect.jade', {
     locals: {
       redirectLink: '/honeydConfigManage'
     }
    });
  } 
  else
  {
    RenderError(res, 'Unacceptable characters in new configuration name', '/honeydConfigManage');
  }
});

app.post('/customizeTrainingSave', function (req, res)
{
    for (var uid in req.body)
    {
        trainingDb.SetIncluded(uid, true);
    }

    trainingDb.Save();

    res.render('saveRedirect.jade', {
        locals: {
            redirectLink: "/customizeTraining"
        }
    })
});

everyone.now.addScriptOptionValue = function (script, key, value, callback) {
    honeydConfig.AddScriptOptionValue(script, key, value);
    honeydConfig.SaveAll();
    callback();
};

everyone.now.deleteScriptOptionValue = function (script, key, value, callback) {
    honeydConfig.DeleteScriptOptionValue(script, key, value);
    honeydConfig.SaveAll();
    callback();
};

everyone.now.createHoneydNodes = function(ipType, ip1, ip2, ip3, ip4, profile, portSet, vendor, interface, count, callback)
{
    var ipAddress;
    if (ipType == "DHCP")
    {
        ipAddress = "DHCP";
    } else {
        ipAddress = ip1 + "." + ip2 + "." + ip3 + "." + ip4;
    }

    var result = null;
    if (!honeydConfig.AddNodes(profile, portSet, vendor, ipAddress, interface, Number(count)))
    {
        result = "Unable to create new nodes";  
    }

    if (!honeydConfig.SaveAll())
    {
        result = "Unable to save honeyd configuration";
    }

    callback(result);
};


everyone.now.SaveDoppelganger = function(node, callback)
{
    var ipAddress = node.ip;
    if (node.ipType == "DHCP")
    {
        ipAddress = "DHCP";
    }

    if (!honeydConfig.SaveDoppelganger(node.profile, node.portSet, ipAddress, node.mac, node.intface))
    {
        callback("SaveDoppelganger Failed");
        return;
    } else {
        if (!honeydConfig.SaveAll())
        {
            callback("Unable to save honeyd configuration");
        } else {
            callback(null);
        }
    }
}

everyone.now.SaveHoneydNode = function(node, callback)
{
    var ipAddress = node.ip;
    if (node.ipType == "DHCP")
    {
        ipAddress = "DHCP";
    }

    // Delete the old node and then add the new one 
    honeydConfig.DeleteNode(node.oldName);

    if (node.oldName == "doppelganger")
    {
        if (!honeydConfig.SetDoppelganger(node.profile, node.portSet, ipAddress, node.mac, node.intface))
        {
            callback("doppelganger Failed");
            return;
        } else {
            if (!honeydConfig.SaveAll())
            {
                callback("Unable to save honeyd configuration");
            } else {
                callback(null);
            }
        }

    } else {
        if (!honeydConfig.AddNode(node.profile, node.portSet, ipAddress, node.mac, node.intface))
        {
            callback("AddNode Failed");
            return;
        } else {
            if (!honeydConfig.SaveAll())
            {
                callback("Unable to save honeyd configuration");
            } else {
                callback(null);
            }
        }
    }

};

app.post('/configureNovaSave', function (req, res)
{
    // TODO: Throw this out and do error checking in the Config (WriteSetting) class instead
    var configItems = ["DEFAULT", "INTERFACE", "SMTP_USER", "SMTP_PASS", "RSYSLOG_IP", "HS_HONEYD_CONFIG", 
    "TCP_TIMEOUT", "TCP_CHECK_FREQ", "READ_PCAP", "PCAP_FILE", "GO_TO_LIVE", "CLASSIFICATION_TIMEOUT", 
    "K", "EPS", "CLASSIFICATION_THRESHOLD", "DATAFILE", "USER_HONEYD_CONFIG", "DOPPELGANGER_IP", 
    "DOPPELGANGER_INTERFACE", "DM_ENABLED", "ENABLED_FEATURES", "THINNING_DISTANCE", "SAVE_FREQUENCY", 
    "DATA_TTL", "CE_SAVE_FILE", "SMTP_ADDR", "SMTP_PORT", "SMTP_DOMAIN", "SMTP_USEAUTH", "RECIPIENTS", 
    "SERVICE_PREFERENCES", "HAYSTACK_STORAGE", "CAPTURE_BUFFER_SIZE", "MIN_PACKET_THRESHOLD", "CUSTOM_PCAP_FILTER", 
    "CUSTOM_PCAP_MODE", "WEB_UI_PORT", "CLEAR_AFTER_HOSTILE_EVENT", "MASTER_UI_IP", "MASTER_UI_RECONNECT_TIME", 
    "MASTER_UI_CLIENT_ID", "MASTER_UI_ENABLED", "CAPTURE_BUFFER_SIZE", "FEATURE_WEIGHTS", "CLASSIFICATION_ENGINE", 
    "THRESHOLD_HOSTILE_TRIGGERS", "ONLY_CLASSIFY_HONEYPOT_TRAFFIC", "EMAIL_ALERTS_ENABLED", "TRAINING_DATA_PATH"];

    Validator.prototype.error = function (msg)
    {
        this._errors.push(msg);
    }

    Validator.prototype.getErrors = function ()
    {
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
  
  if(req.body["EMAIL_ALERTS_ENABLED"] == undefined)
  {
    req.body["EMAIL_ALERTS_ENABLED"] = "0";
    config.WriteSetting("EMAIL_ALERTS_ENABLED", "0");
  }
  else
  {
    req.body["EMAIL_ALERTS_ENABLED"] = "1";
    config.WriteSetting("EMAIL_ALERTS_ENABLED", "1");
  }
  
  if(req.body["DM_ENABLED"] == undefined)
  {
    req.body["DM_ENABLED"] = "0";
    config.WriteSetting("DM_ENABLED", "0");
  }
  else
  {
    req.body["DM_ENABLED"] = "1";
    config.WriteSetting("DM_ENABLED", "1");
  }

  if(clientId != undefined && req.body["MASTER_UI_CLIENT_ID"] != clientId)
  {
    if(pulsar != undefined)
    {
      var renameMessage = {};
      renameMessage.type = 'renameRequest';
      renameMessage.id = clientId;
      renameMessage.newId = req.body["MASTER_UI_CLIENT_ID"];
      pulsar.sendUTF(JSON.stringify(renameMessage));
      clientId = req.body["MASTER_UI_CLIENT_ID"];
    }
  }

    var interfaces = "";
    var oneIface = false;

    if(req.body["DOPPELGANGER_INTERFACE"] !== undefined) 
    {
        config.SetDoppelInterface(req.body["DOPPELGANGER_INTERFACE"]);
    }

    if(req.body["INTERFACE"] !== undefined) 
    {
        for(item in req.body["INTERFACE"]) 
        {
            if(req.body["INTERFACE"][item].length > 1) 
            {
                interfaces += " " + req.body["INTERFACE"][item];
                config.AddIface(req.body["INTERFACE"][item]);
            } 
            else 
            {
                interfaces += req.body["INTERFACE"][item];
                oneIface = true;
            }
        }

        if (oneIface) 
        {
            config.AddIface(interfaces);
        }

        req.body["INTERFACE"] = interfaces;
    }

    for (var item = 0; item < configItems.length; item++) 
    {
        if (req.body[configItems[item]] == undefined) 
        {
            continue;
        }
        switch (configItems[item])
        {
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

    case "RSYSLOG_IP":
      if(/^((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\.){3}(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})(\:[0-9]+)?$/.test(req.body[configItems[item]]) == false 
         && req.body[configItems[item]] != 'NULL')
      {
        validator.check(req.body[configItems[item]], 'Invalid format for Rsyslog server IP, must be IP:Port or the string "NULL"').regex('^((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\.){3}(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})(\:[0-9]+)?$');
      }
      break;
      
        case "DOPPELGANGER_IP":
            validator.check(req.body[configItems[item]], 'Doppelganger IP must be in the correct IP format').regex('^((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\.){3}(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})$');
            var split = req.body[configItems[item]].split('.');

            if (split.length == 4) 
            {
                if (split[3] === "0") 
                {
                    validator.check(split[3], 'Can not have last IP octet be 0').equals("255");
                }
                if (split[3] === "255") 
                {
                    validator.check(split[3], 'Can not have last IP octet be 255').equals("0");
                }
            }

            //check for 0.0.0.0 and 255.255.255.255
            var checkIPZero = 0;
            var checkIPBroad = 0;

            for (var val = 0; val < split.length; val++) 
            {
                if (split[val] == "0") 
                {
                    checkIPZero++;
                }
                if (split[val] == "255") 
                {
                    checkIPBroad++;
                }
            }

            if (checkIPZero == 4)
            {
                validator.check(checkIPZero, '0.0.0.0 is not a valid IP address').is("200");
            }
            if (checkIPBroad == 4)
            {
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
            validator.check(req.body[configItems[item]], "Service Preferences string is formatted incorrectly").is('^0:[0-7](\\+|\\-)?;1:[0-7](\\+|\\-)?;$');
            break;

        default:
            break;
        }
    }

    var errors = validator.getErrors();

  var writeIP = req.body["RSYSLOG_IP"];

  if(writeIP != undefined && writeIP != getRsyslogIp() && writeIP != 'NULL')
  {
    var util = require('util');
    var spawn = require('sudo');
    var options = {
      cachePassword: true
      , prompt: 'You have requested to change the target server for Rsyslog. This requires superuser permissions.'
    };
  
    var execution = ['nova_rsyslog_helper', writeIP.toString()];
  
    var rsyslogHelper = spawn(execution, options);

    rsyslogHelper.on('exit', function(code){
      if(code.toString() != '0')
      {
        console.log('nova_rsyslog_helper could not complete update of rsyslog configuration, exited with code ' + code);
      }
      else
      {
        console.log('nova_rsyslog_helper updated rsyslog configuration');
      }
    });
  }
  else if(writeIP == 'NULL')
  {
    var util = require('util');
    var spawn = require('sudo');
    var options = {
      cachePassword: true
      , prompt: 'You have requested to change the target server for Rsyslog. This requires superuser permissions.'
    };
    
    var execution = ['nova_rsyslog_helper','remove'];
    var rm = spawn(execution, options); 
    rm.on('exit', function(code){
      console.log('41-nova.conf has been removed from /etc/rsyslog.d/');
    });
  }

    if (errors.length > 0)
    {
        RenderError(res, errors, "/suspects");
    } 
    else 
    {
        if (req.body["INTERFACE"] !== undefined && req.body["DEFAULT"] === undefined)
        {
            req.body["DEFAULT"] = false;
            config.UseAllInterfaces(false);
            config.WriteSetting("INTERFACE", req.body["INTERFACE"]);
        } else if (req.body["INTERFACE"] === undefined)
        {
            req.body["DEFAULT"] = true;
            config.UseAllInterfaces(true);
            config.WriteSetting("INTERFACE", "default");
        } else {
            config.UseAllInterfaces(req.body["DEFAULT"]);
            config.WriteSetting("INTERFACE", req.body["INTERFACE"]);
        }

        if (req.body["SMTP_USER"] !== undefined)
        {
            config.SetSMTPUser(req.body["SMTP_USER"]);
        }
        if (req.body["SMTP_PASS"] !== undefined)
        {
            config.SetSMTPPass(req.body["SMTP_PASS"]);
        }

        //if no errors, send the validated form data to the WriteSetting method
        for (var item = 5; item < configItems.length; item++)
        {
            if (req.body[configItems[item]] != undefined)
            {
                config.WriteSetting(configItems[item], req.body[configItems[item]]);
            }
        }

        var route = "/suspects";
        if (req.body['route'] != undefined)
        {
            route = req.body['route'];
            if (route == 'manconfig')
            {
                route = 'honeydConfigManage';
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

function GetPorts()
{
  var scriptBindings = {};
  
  var profiles = honeydConfig.GetProfileNames();
  
  for(var i in profiles)
  {
    var profileName = profiles[i];

    var portSets = honeydConfig.GetPortSetNames(profiles[i]);
    for (var portSetName in portSets)
    {
        var portSet = honeydConfig.GetPortSet(profiles[i], portSets[portSetName]);
        var ports = [];
        for (var p in portSet.GetTCPPorts())
        {
            ports.push(portSet.GetTCPPorts()[p]);
        }
        for (var p in portSet.GetUDPPorts())
        {
            ports.push(portSet.GetUDPPorts()[p]);
        }
        for(var p in ports)
        {
            if(ports[p].GetScriptName() != undefined && ports[p].GetScriptName() != '')
            {
                if(scriptBindings[ports[p].GetScriptName()] == undefined)
                {
                    scriptBindings[ports[p].GetScriptName()] = profileName + "(" + portSet.GetName() + ")" + ':' + ports[p].GetPortNum();
                } else {
                    scriptBindings[ports[p].GetScriptName()] += '<br>' + profileName + "(" + portSet.GetName() + ")" + ':' + ports[p].GetPortNum();
                }
            }
        }
    }
  }

  return scriptBindings;
}

app.get('/scripts', function(req, res){
  var namesAndPaths = [];
  
  var scriptNames = honeydConfig.GetScriptNames();

  for (var i = 0; i < scriptNames.length; i++) {
    var script = honeydConfig.GetScript(scriptNames[i]);
    namesAndPaths.push({script: script.GetName(), path: script.GetPath(), configurable: script.GetIsConfigurable()});
  }
  

  function cmp(a, b)
  {
    return a.script.localeCompare(b.script);
  }
  
  namesAndPaths.sort(cmp);
  var scriptBindings = GetPorts(); 
  
  res.render('scripts.jade', {
    locals: {
      scripts: namesAndPaths,
      bindings: scriptBindings
    }
  });
});

everyone.now.ClearAllSuspects = function (callback)
{
    nova.CheckConnection();
    if (!nova.ClearAllSuspects())
    {
        console.log("Manually deleting CE state file:" + NovaHomePath + "/" + config.ReadSetting("CE_SAVE_FILE"));
        // If we weren't able to tell novad to clear the suspects, at least delete the CEStateFile
        try {
            fs.unlinkSync(NovaHomePath + "/" + config.ReadSetting("CE_SAVE_FILE"));
        } catch (err)
        {
            // this is probably because the file doesn't exist. Just ignore.
        }
    }
}

everyone.now.ClearSuspect = function (suspectIp, interface, callback)
{
    nova.CheckConnection();
    var result = nova.ClearSuspect(suspectIp, interface);

    if (callback != undefined)
    {
        callback(result);
    }
}

everyone.now.GetInheritedEthernetList = function (parent, callback)
{
    var prof = honeydConfig.GetProfile(parent);

    if (prof == null)
    {
        console.log("ERROR Getting profile " + parent);
        callback(null);
    } else {
        callback(prof.GetVendors(), prof.GetVendorCounts());
    }

}

everyone.now.RestartHaystack = function(cb)
{
    nova.StopHaystack();

    // Note: the other honeyd may be shutting down still,
    // but the slight overlap doesn't cause problems
    nova.StartHaystack(false);

    if(typeof callback == 'function')
    {
        cb();
    }
}

everyone.now.StartHaystack = function()
{
    if(!nova.IsHaystackUp())
    {
        nova.StartHaystack(false);
    }
  if(!nova.IsHaystackUp())
  {
    everyone.now.HaystackStartFailed();
  }
  else
  {
    try 
    {
        everyone.now.updateHaystackStatus(nova.IsHaystackUp())
    } 
    catch(err)
    {};
    }
}

everyone.now.StopHaystack = function()
{
    nova.StopHaystack();
    try 
    {
        everyone.now.updateHaystackStatus(nova.IsHaystackUp());
    } 
    catch(err)
    {};
}

everyone.now.IsHaystackUp = function(callback)
{
    callback(nova.IsHaystackUp());
}

everyone.now.IsNovadUp = function(callback)
{
    callback(nova.IsNovadUp(false));
}

everyone.now.StartNovad = function()
{
    var result = nova.StartNovad(false);
    result = nova.CheckConnection();
    try 
    {
        everyone.now.updateNovadStatus(nova.IsNovadUp(false));
    }
    catch(err){};
}

everyone.now.StopNovad = function(cb)
{
    if(nova.StopNovad() == false)
    {
    cb('false');
    return;
    }
    nova.CloseNovadConnection();
    try 
    {
        everyone.now.updateNovadStatus(nova.IsNovadUp(false));
    }
    catch(err){};
}

everyone.now.HardStopNovad = function(passwd)
{
  nova.HardStopNovad(passwd);
  nova.CloseNovadConnection();
  try 
  {
    everyone.now.updateNovadStatus(nova.IsNovadUp(false));
  }
  catch(err){};
}

everyone.now.sendAllSuspects = function (callback)
{
    nova.CheckConnection();
    nova.sendSuspectList(callback);
}

everyone.now.sendSuspect = function (interface, ip, callback)
{
    var suspect = nova.sendSuspect(interface, ip);
    if (suspect.GetIdString === undefined)
    {
        console.log("Failed to get suspect");
        return;
    }
    var s = new Object();
    objCopy(suspect, s);
    callback(s);
}


everyone.now.deleteUserEntry = function (usernamesToDelete, callback)
{
    var username;
    for (var i = 0; i < usernamesToDelete.length; i++)
    {
        username = String(usernamesToDelete[i]);
        dbqCredentialsDeleteUser.run(username, function (err)
        {
            if (err)
            {
                console.log("Database error: " + err);
                callback(false);
                return;
            }
            else
            {
                callback(true);
            }
        });
    }
}

everyone.now.updateUserPassword = function (username, newPassword, callback)
{
  var salt = '';
  var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  for(var i = 0; i < 8; i++)
  {
    salt += possible[Math.floor(Math.random() * possible.length)];
  }
  
  //update credentials set pass=? and salt=? where user=?
  dbqCredentialsChangePassword.run(HashPassword(newPassword, salt), salt, username, function(err){
    console.log('err ' + err);
    if(err)
    {
      callback(false);
    }
    else
    {
      callback(true);
    }
  });
};

// Deletes a honeyd node
everyone.now.deleteNodes = function (nodeNames, callback)
{
    var nodeName;
    for (var i = 0; i < nodeNames.length; i++)
    {
        nodeName = nodeNames[i];
        if (nodeName != null && !honeydConfig.DeleteNode(nodeName))
        {
            callback(false, "Failed to delete node " + nodeName);
            return;
        }

    }

    if (!honeydConfig.SaveAll())
    {
        callback(false, "Failed to save XML templates");
        return;
    }

    callback(true, "");
}

everyone.now.deleteProfiles = function (profileNames, callback)
{
    var profileName;
    for (var i = 0; i < profileNames.length; i++)
    {
        profileName = profileNames[i];

        if (!honeydConfig.DeleteProfile(profileName))
        {
            callback(false, "Failed to delete profile " + profileName);
            return;
        }


        if (!honeydConfig.SaveAll())
        {
            callback(false, "Failed to save XML templates");
            return;
        }
    }

    callback(true, "");
}

everyone.now.addWhitelistEntry = function (interface, entry, callback)
{
    // TODO: Input validation. Should be IP address or 'IP/netmask'
    // Should also be sanitized for newlines/trailing whitespace
    if (whitelistConfig.AddEntry(interface + "," + entry))
    {
        callback(true, "");
    } else {
        callback(true, "Attempt to add whitelist entry failed");
    }
}

everyone.now.deleteWhitelistEntry = function (whitelistEntryNames, callback)
{
    var whitelistEntryName;
    for (var i = 0; i < whitelistEntryNames.length; i++)
    {
        whitelistEntryName = whitelistEntryNames[i];

        if (!whitelistConfig.DeleteEntry(whitelistEntryName))
        {
            callback(false, "Failed to delete whitelistEntry " + whitelistEntryName);
            return;
        }
    }

    callback(true, "");
}

everyone.now.GetProfile = function (profileName, callback)
{
    var profile = honeydConfig.GetProfile(profileName);

    
    if (profile == null)
    {
        callback(null);
        return;
    }

    // Nowjs can't pass the object with methods, they need to be member vars
    profile.name = profile.GetName();
    profile.personality = profile.GetPersonality();

    profile.uptimeMin = profile.GetUptimeMin();
    profile.uptimeMax = profile.GetUptimeMax();
    profile.dropRate = profile.GetDropRate();
    profile.parentProfile = profile.GetParentProfile();

    profile.isPersonalityInherited = profile.IsPersonalityInherited();
    profile.isUptimeInherited = profile.IsUptimeInherited();
    profile.isDropRateInherited = profile.IsDropRateInherited();

    profile.count = profile.GetCount();

        profile.portSets = GetPortSets(profileName);


    var ethVendorList = [];

    var profVendors = profile.GetVendors();
    var profCounts = profile.GetVendorCounts();

    for (var i = 0; i < profVendors.length; i++)
    {
        var element = {};
        element.vendor = profVendors[i];
        element.count = profCounts[i];
        ethVendorList.push(element);
    }

    profile.ethernet = ethVendorList;


    callback(profile);
}

everyone.now.GetScript = function (scriptName, callback)
{
    var script = honeydConfig.GetScript(scriptName);
    var methodlessScript = {};

    objCopy(script, methodlessScript);

    callback(methodlessScript);

}

everyone.now.GetVendors = function (profileName, callback)
{
    var profile = honeydConfig.GetProfile(profileName);

    if (profile == null)
    {
        console.log("ERROR Getting profile " + profileName);
        callback(null);
        return;
    }


    var ethVendorList = [];

    var profVendors = profile.GetVendors();
    var profDists = profile.GetVendorCounts();

    for (var i = 0; i < profVendors.length; i++)
    {
        var element = {
            vendor: "",
            count: ""
        };
        element.vendor = profVendors[i];
        element.count = parseFloat(profDists[i]);
        ethVendorList.push(element);
    }

    callback(profVendors, profDists);
}

GetPortSets = function (profileName, callback)
{
    var portSetNames = honeydConfig.GetPortSetNames(profileName);
    
    var portSets = [];  

    for (var i = 0; i < portSetNames.length; i++)
    {
        var portSet = honeydConfig.GetPortSet( profileName, portSetNames[i] );
        portSet.setName = portSet.GetName();
        portSet.TCPBehavior = portSet.GetTCPBehavior();
        portSet.UDPBehavior = portSet.GetUDPBehavior();
        portSet.ICMPBehavior = portSet.GetICMPBehavior();

        portSet.TCPExceptions = portSet.GetTCPPorts();
        for (var j = 0; j < portSet.TCPExceptions.length; j++)
        {
            portSet.TCPExceptions[j].portNum = portSet.TCPExceptions[j].GetPortNum();
            portSet.TCPExceptions[j].protocol = portSet.TCPExceptions[j].GetProtocol();
            portSet.TCPExceptions[j].behavior = portSet.TCPExceptions[j].GetBehavior();
            portSet.TCPExceptions[j].scriptName = portSet.TCPExceptions[j].GetScriptName();
            portSet.TCPExceptions[j].service = portSet.TCPExceptions[j].GetService();
            portSet.TCPExceptions[j].scriptConfiguration = portSet.TCPExceptions[j].GetScriptConfiguration();
        }

        portSet.UDPExceptions = portSet.GetUDPPorts();
        for (var j = 0; j < portSet.UDPExceptions.length; j++)
        {
            portSet.UDPExceptions[j].portNum = portSet.UDPExceptions[j].GetPortNum();
            portSet.UDPExceptions[j].protocol = portSet.UDPExceptions[j].GetProtocol();
            portSet.UDPExceptions[j].behavior = portSet.UDPExceptions[j].GetBehavior();
            portSet.UDPExceptions[j].scriptName = portSet.UDPExceptions[j].GetScriptName();
            portSet.UDPExceptions[j].scriptConfiguration = portSet.UDPExceptions[j].GetScriptConfiguration();
        }
        portSets.push(portSet);
    }

    if(typeof callback == 'function')
    {
    callback(portSets, profileName);
  }
  return portSets;
}
everyone.now.GetPortSets = GetPortSets;


function jsProfileToHoneydProfile(profile)
{
    var honeydProfile = new novaconfig.HoneydProfileBinding(profile.parentProfile, profile.name);
    
        //Set Ethernet vendors
    var ethVendors = [];
    var ethDists = [];

    for (var i in profile.ethernet)
    {
        ethVendors.push(profile.ethernet[i].vendor);
        ethDists.push(parseFloat(Number(profile.ethernet[i].count)));
    }
    honeydProfile.SetVendors(ethVendors, ethDists);
    

    // Move the Javascript object values to the C++ object
    honeydProfile.SetUptimeMin(Number(profile.uptimeMin));
    honeydProfile.SetUptimeMax(Number(profile.uptimeMax));
    honeydProfile.SetDropRate(Number(profile.dropRate));
    honeydProfile.SetPersonality(profile.personality);
    honeydProfile.SetCount(profile.count);

    honeydProfile.SetIsPersonalityInherited(Boolean(profile.isPersonalityInherited));
    honeydProfile.SetIsDropRateInherited(Boolean(profile.isDropRateInherited));
    honeydProfile.SetIsUptimeInherited(Boolean(profile.isUptimeInherited));


    // Add new ports
    honeydProfile.ClearPorts();
    var portName;
    for (var i = 0; i < profile.portSets.length; i++) 
    {
        //Make a new port set
		var encodedName = sanitizeCheck(profile.portSets[i].setName).entityEncode();
        honeydProfile.AddPortSet(encodedName);

        honeydProfile.SetPortSetBehavior(encodedName, "tcp", profile.portSets[i].TCPBehavior);
        honeydProfile.SetPortSetBehavior(encodedName, "udp", profile.portSets[i].UDPBehavior);
        honeydProfile.SetPortSetBehavior(encodedName, "icmp", profile.portSets[i].ICMPBehavior);

        for (var j = 0; j < profile.portSets[i].TCPExceptions.length; j++)
        {
            var scriptConfigKeys = new Array();
            var scriptConfigValues = new Array();

            for (var key in profile.portSets[i].TCPExceptions[j].scriptConfiguration)
            {
                scriptConfigKeys.push(key);
                scriptConfigValues.push(profile.portSets[i].TCPExceptions[j].scriptConfiguration[key]);
            }

            honeydProfile.AddPort(encodedName,
                    profile.portSets[i].TCPExceptions[j].behavior, 
                    profile.portSets[i].TCPExceptions[j].protocol, 
                    Number(profile.portSets[i].TCPExceptions[j].portNum), 
                    profile.portSets[i].TCPExceptions[j].scriptName,
                    scriptConfigKeys,
                    scriptConfigValues);
        }

        for (var j = 0; j < profile.portSets[i].UDPExceptions.length; j++)
        {
            var scriptConfigKeys = new Array();
            var scriptConfigValues = new Array();

            for (var key in profile.portSets[i].UDPExceptions[j].scriptConfiguration)
            {
                scriptConfigKeys.push(key);
                scriptConfigValues.push(profile.portSets[i].UDPExceptions[j].scriptConfiguration[key]);
            }

            honeydProfile.AddPort(encodedName, 
                    profile.portSets[i].UDPExceptions[j].behavior, 
                    profile.portSets[i].UDPExceptions[j].protocol, 
                    Number(profile.portSets[i].UDPExceptions[j].portNum), 
                    profile.portSets[i].UDPExceptions[j].scriptName,
                    scriptConfigKeys,
                    scriptConfigValues);
        }

    }

    return honeydProfile;
}


//portSets = A 2D array. (array of portSets, which are arrays of Ports)
everyone.now.SaveProfile = function (profile, callback)
{
    // Check input
    var profileNameRegexp = new RegExp("[a-zA-Z]+[a-zA-Z0-9 ]*");
    var match = profileNameRegexp.exec(profile.name);
    
    if (match == null) 
    {
        var err = "ERROR: Attempt to save a profile with an invalid name. Must be alphanumeric and not begin with a number.";
        callback(err);
        return;
    }


    var honeydProfile = jsProfileToHoneydProfile(profile);
    honeydProfile.Save();

    // Save the profile
    if (!honeydConfig.SaveAll())
    {
        result = "Unable to save honeyd configuration";
    }

    callback();
}

everyone.now.RenamePortset = function(profile, oldName, newName, callback)
{
  var encodedName = sanitizeCheck(newName).entityEncode();
  var result = honeydConfig.RenamePortset(oldName, encodedName, profile);
  honeydConfig.SaveAll();
  if(typeof callback == 'function')
  {
    callback();
  }
}

everyone.now.WouldProfileSaveDeleteNodes = function (profile, callback)
{
    var honeydProfile = jsProfileToHoneydProfile(profile);

    callback(honeydProfile.WouldAddProfileCauseNodeDeletions());
}

everyone.now.GetCaptureSession = function (callback)
{
    var ret = config.ReadSetting("TRAINING_SESSION");
    callback(ret);
}

everyone.now.ShowAutoConfig = function (nodeInterface, numNodesType, numNodes, interfaces, subnets, groupName, append, callback, route)
{
    var executionString = 'haystackautoconfig';

    var hhconfigArgs = new Array();


	hhconfigArgs.push('--nodeinterface');
	hhconfigArgs.push(nodeInterface);

    if(numNodesType == "fixed") 
    {
        if(numNodes !== undefined) 
        {
            hhconfigArgs.push('-n');
            hhconfigArgs.push(numNodes);
        }
    } 
    else if(numNodesType == "ratio") 
    {
        if(numNodes !== undefined) 
        {
            hhconfigArgs.push('-r');
            hhconfigArgs.push(numNodes);
        }
    }
    else if(numNodesType == 'range')
    {
      if(numNodes !== undefined)
      {
        hhconfigArgs.push('e');
        hhconfigArgs.push(numNodes);
      }
    }
    
    if(interfaces !== undefined && interfaces.length > 0)
    {
        hhconfigArgs.push('-i');
        hhconfigArgs.push(interfaces);
    }
    if(subnets !== undefined && subnets.length > 0)
    {
        hhconfigArgs.push('-a');
        hhconfigArgs.push(subnets);
    }
    if(groupName !== undefined && groupName !== '*')
    {
      hhconfigArgs.push('-g');
      hhconfigArgs.push(groupName);
      honeydConfig.AddConfiguration(groupName, 'false', '');
    config.SetCurrentConfig(groupName);
    }
    if(append !== undefined && append !== '*')
    {
      hhconfigArgs.push('t');
      hhconfigArgs.push(append);
    }

    var util = require('util');
    var spawn = require('child_process').spawn;

    autoconfig = spawn(executionString.toString(), hhconfigArgs);

    autoconfig.stdout.on('data', function (data)
    {
      if(typeof callback == 'function')
      {
          callback('' + data);
        }
    });

    autoconfig.stderr.on('data', function (data)
    {
        if (/^execvp\(\)/.test(data))
        {
            console.log("haystackautoconfig failed to start.");
            var response = "haystackautoconfig failed to start.";
            everyone.now.SwitchConfigurationTo('default');
            if(typeof route == 'function')
            {
              route("/autoConfig", response);
            }
        }
    });

    autoconfig.on('exit', function (code, signal)
    {
        console.log("autoconfig exited with code " + code);
        var response = "autoconfig exited with code " + code;
      if(typeof route == 'function' && signal != 'SIGTERM')
    {
      route("/honeydConfigManage", response);
    }
    if(signal == 'SIGTERM')
    {
      response = "autoconfig scan terminated early";
      route("/autoConfig", response);
    }
    });
}

everyone.now.CancelAutoScan = function(groupName)
{
  try
  {
    autoconfig.kill();
    autoconfig = undefined;
    everyone.now.RemoveConfiguration(groupName);
    
    everyone.now.SwitchConfigurationTo('default');
  }
  catch(e)
  {
    LOG("ERROR", "CancelAutoScan threw an error: " + e);
  }
}

// TODO: Fix training
everyone.now.StartTrainingCapture = function (trainingSession, callback)
{
    callback("Training mode is currently not supported");
    return;

    //config.WriteSetting("IS_TRAINING", "1");
    config.WriteSetting("TRAINING_SESSION", trainingSession.toString());

    // Check if training folder already exists
    //console.log(Object.keys(fs));
    path.exists(NovaHomePath + "/data/" + trainingSession, function (exists)
    {
        if (exists)
        {
            callback("Training session folder already exists for session name of '" + trainingSession + "'");
            return;
        } else {
            // Start the haystack
            if (!nova.IsHaystackUp())
            {
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
everyone.now.StopTrainingCapture = function (trainingSession, callback)
{
    callback("Training mode is currently not supported");
    return;
    //config.WriteSetting("IS_TRAINING", "0");
    //config.WriteSetting("TRAINING_SESSION", "null");
    nova.StopNovad();

    exec('novatrainer ' + NovaHomePath + '/data/' + trainingSession + ' ' + NovaHomePath + '/data/' + trainingSession + '/nova.dump',

    function (error, stdout, stderr)
    {
        callback(stderr);
    });
}

everyone.now.GetCaptureIPs = function (trainingSession, callback)
{
    return trainingDb.GetCaptureIPs(NovaHomePath + "/data/" + trainingSession + "/nova.dump");
}

everyone.now.WizardHasRun = function (callback)
{
    dbqFirstrunInsert.run(callback);
}


everyone.now.GetHostileEvents = function (callback)
{
    dbqSuspectAlertsGet.all(

    function (err, results)
    {
        if (err)
        {
            console.log("Database error: " + err);
            // TODO implement better error handling callbacks
            callback();
            return;
        }

        callback(results);
    });
}

everyone.now.ClearHostileEvents = function (callback)
{
    dbqSuspectAlertsDeleteAll.run(

    function (err)
    {
        if (err)
        {
            console.log("Database error: " + err);
            // TODO implement better error handling callbacks
            return;
        }

        callback("true");
    });
}

everyone.now.ClearHostileEvent = function (id, callback)
{
    dbqSuspectAlertsDeleteAlert(id,

    function (err)
    {
        if (err)
        {
            console.log("Database error: " + err);
            // TODO implement better error handling callbacks
            return;
        }

        callback("true");
    });
}

var SendHostileEventToPulsar  = function(suspect)
{
  suspect.client = clientId;
  suspect.type = 'hostileSuspect';
  if(pulsar != undefined)
  {
    pulsar.sendUTF(JSON.stringify(suspect));
  }
};
everyone.now.SendHostileEventToPulsar = SendHostileEventToPulsar;

var SendBenignSuspectToPulsar = function(suspect)
{
  suspect.client = clientId;
  suspect.type = 'benignSuspect';
  if(pulsar != undefined)
  {
    pulsar.sendUTF(JSON.stringify(suspect));
  }
};
everyone.now.SendBenignSuspectToPulsar = SendBenignSuspectToPulsar;

everyone.now.GetLocalIP = function (iface, callback)
{
    callback(nova.GetLocalIP(iface));
}

everyone.now.GetSubnetFromInterface = function (iface, index, callback)
{
  callback(iface, nova.GetSubnetFromInterface(iface), index);
}

everyone.now.RemoveScriptFromProfiles = function(script, callback)
{
    honeydConfig.DeleteScriptFromPorts(script);

    honeydConfig.SaveAllTemplates();

    if(typeof callback == 'function')
    {
        callback();
    }
}

everyone.now.GenerateMACForVendor = function(vendor, callback)
{
    callback(honeydConfig.GenerateRandomUnusedMAC(vendor));
}

everyone.now.restoreDefaultSettings = function(callback)
{
    var source = NovaSharedPath + "/../userFiles/config/NOVAConfig.txt";
    var destination = NovaHomePath + "/config/NOVAConfig.txt";
    exec('cp -f ' + source + ' ' + destination, function(err)
    {
        callback();
    }); 
}

everyone.now.reverseDNS = function(ip, callback)
{
    dns.reverse(ip, callback);
}

everyone.now.addTrainingPoint = function(ip, interface, features, hostility, callback)
{
    if (hostility != '0' && hostility != '1')
    {
        callback("Error: Invalid hostility. Should be 0 or 1");
        return;
    }
	
	if (features.toString().split(" ").length != nova.GetDIM()) {
		callback("Error: Invalid number of features!")
		return;
	}

    var point = features.toString() + " " + hostility + "\n";
    fs.appendFile(NovaHomePath + "/config/training/data.txt", point, function(err)
    {
        if (err)
        {
            console.log("Error: " + err);
            callback(err);
            return;
        }

        var d = new Date();
        var trainingDbString = "";
        trainingDbString += hostility + ' "User customized training point for suspect ' + ip + " added on " + d.toString() + '"\n';
        trainingDbString += "\t" + features.toString();
        trainingDbString += "\n\n";

        fs.appendFile(NovaHomePath + "/config/training/training.db", trainingDbString, function(err)
        {
            if (!nova.ReclassifyAllSuspects())
            {
                callback("Error: Unable to reclassify suspects with new training data");
                return;
            }
            callback();
        });

    }); 
}

everyone.now.RemoveScript = function(scriptName, callback)
{
  honeydConfig.RemoveScript(scriptName);
  
  honeydConfig.SaveAllTemplates();
  
  if(typeof callback == 'function')
  {
    callback();
  }
}

everyone.now.WriteHoneydConfig = function(cb)
{
   honeydConfig.WriteHoneydConfiguration(config.GetPathConfigHoneydHS());
   
   if(typeof cb == 'function')
   {
     cb(); 
   }
}

everyone.now.GetConfigSummary = function(configName, callback)
{ 
  honeydConfig.LoadAllTemplates();
  
  var scriptProfileBindings = GetPorts();
  var profiles = honeydConfig.GetProfileNames();
  var profileObj = {};
  
  for (var i = 0; i < profiles.length; i++) 
  {
    if(profiles[i] != undefined && profiles[i] != '')
    {
      var prof = honeydConfig.GetProfile(profiles[i]);
      var obj = {};
      var vendorNames = prof.GetVendors();
      var vendorDist = prof.GetVendorCounts();
      
      obj.name = prof.GetName();
      obj.parent = prof.GetParentProfile();
      obj.os = prof.GetPersonality();
      obj.packetDrop = prof.GetDropRate();
      obj.vendors = [];
      
      for(var j = 0; j < vendorNames.length; j++)
      {
        var push = {};
        
        push.name = vendorNames[j];
        push.count = vendorDist[j];
        obj.vendors.push(push);
      }
      
      if(prof.GetUptimeMin() == prof.GetUptimeMax())
      {
        obj.fixedOrRange = 'fixed';
        obj.uptimeValue = prof.GetUptimeMin();
      }
      else
      {
        obj.fixedOrRange = 'range';
        obj.uptimeValueMin = prof.GetUptimeMin();
        obj.uptimeValueMax = prof.GetUptimeMax();        
      }
      
      //obj.defaultTCP = prof.GetTcpAction();
      //obj.defaultUDP = prof.GetUdpAction();
      //obj.defaultICMP = prof.GetIcmpAction();
      profileObj[profiles[i]] = obj;
    }
  }
  
  var nodeNames = honeydConfig.GetNodeMACs();
  var nodeList = [];
  
  for (var i = 0; i < nodeNames.length; i++)
  {
    var node = honeydConfig.GetNode(nodeNames[i]);
    var push = cNodeToJs(node);
    nodeList.push(push);
  }
  
  nodeNames = null;
  
  if(typeof callback == 'function')
  {
    callback(scriptProfileBindings, profileObj, profiles, nodeList);
  }
}

everyone.now.SwitchConfigurationTo = function(configName)
{
	honeydConfig.SwitchConfiguration(configName); 
    config.WriteSetting('CURRENT_CONFIG', configName);
}

everyone.now.RemoveConfiguration = function(configName, callback)
{
  if(configName == 'default')
  {
    console.log('Cannot delete default configuration');
  }
  
  honeydConfig.RemoveConfiguration(configName);
  
  if(typeof callback == 'function')
  {
    callback(configName);
  }
}

var distributeSuspect = function (suspect)
{
    var s = new Object();
    
    objCopy(suspect, s);
    s.interfaceAlias = ConvertInterfaceToAlias(s.GetInterface);
    
    try 
    {
        everyone.now.OnNewSuspect(s);
    } catch (err) {};
  
  if(suspect.GetIsHostile() == true && parseInt(suspect.GetClassification()) >= 0)
  {
    var d = new Date(suspect.GetLastPacketTime() * 1000);
    var dString = pad(d.getMonth() + 1) + "/" + pad(d.getDate()) + " " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds());
    var send = {};
    
    send.ip = suspect.GetIpString();
    send.classification = String(suspect.GetClassification());
    send.lastpacket = dString;
    send.ishostile = String(suspect.GetIsHostile());
    send.interface = String(suspect.GetInterface());
    
    SendHostileEventToPulsar(send);
  }
  else if(suspect.GetIsHostile() == false && benignRequest && parseInt(suspect.GetClassification()) >= 0)
  {
    var d = new Date(suspect.GetLastPacketTime() * 1000);
    var dString = pad(d.getMonth() + 1) + "/" + pad(d.getDate()) + " " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds());
    var send = {};
    
    send.ip = suspect.GetIpString();
    send.classification = String(suspect.GetClassification());
    send.lastpacket = dString;
    send.ishostile = String(suspect.GetIsHostile());
    send.interface = String(suspect.GetInterface());
    
    SendBenignSuspectToPulsar(send);
  }
  else
  {

  }
};

var distributeAllSuspectsCleared = function ()
{
    try 
    {
        everyone.now.AllSuspectsCleared();
    } 
    catch(err) 
    {
        // We can safely ignore this, it's just because no browsers are connected
    };
}

var distributeSuspectCleared = function (suspect)
{
    var s = new Object;
    
    s['interface'] = suspect.GetInterface();
    s['ip'] = suspect.GetIpString();
    s['idString'] = suspect.GetIdString();
    
    everyone.now.SuspectCleared(s);
}

nova.registerOnAllSuspectsCleared(distributeAllSuspectsCleared);
nova.registerOnSuspectCleared(distributeSuspectCleared);
nova.registerOnNewSuspect(distributeSuspect);


process.on('SIGINT', function ()
{
    nova.Shutdown();
    process.exit();
});

process.on('exit', function ()
{
    LOG("ALERT", "Quasar is exiting cleanly.");
});

function objCopy(src, dst) 
{
    for (var member in src) 
    {
        if (typeof src[member] == 'function') 
        {
            dst[member] = src[member]();
        }
        // Need to think about this
        //        else if ( typeof src[member] == 'object' )
        //        {
        //            copyOver(src[member], dst[member]);
        //        }
        else 
        {
            dst[member] = src[member];
        }
    }
}

function switcher(err, user, success, done) 
{
    if (!success) 
    {
        return done(null, false, {
            message: 'Username/password combination is incorrect'
        });
    }
    return done(null, user);
}

function pad(num) 
{
  return ("0" + num.toString()).slice(-2);
}

function getRsyslogIp()
{
  try
  {
    var read = fs.readFileSync('/etc/rsyslog.d/41-nova.conf', 'utf8');
    read = read.replace(/\n/,' ');
  }
  catch(err)
  {
    return 'NULL'; 
  }
  
  var idx = parseInt(read.indexOf('@@')) + 2;
  var ret = '';
   
  if(idx != -1)
  {
    ret = read.substring(parseInt(idx), parseInt(read.indexOf(':programname', idx)));
  }  
  
  return ret;
}

everyone.now.AddInterfaceAlias = function(iface, alias, callback)
{
    if (alias != "") 
    {
        interfaceAliases[iface] = sanitizeCheck(alias).entityEncode();
    } 
    else 
    {
        delete interfaceAliases[iface];
    }

    var fileString = JSON.stringify(interfaceAliases);
    fs.writeFile(NovaHomePath + "/config/interface_aliases.txt", fileString, callback);
}

everyone.now.GetHaystackDHCPStatus = function(callback)
{
    fs.readFile("/var/log/honeyd/ipList", 'utf8', function (err, data)
    {
        var DHCPIps = new Array();
        if (err)
        {
            RenderError(res, "Unable to open Honeyd status file for reading due to error: " + err);
            return;
        } else {

            data = data.toString().split("\n");
            for(var i = 0; i < data.length; i++)
            {
                if (data[i] == "") {continue};
                var entry = {
                    ip: data[i].toString().split(",")[0],
                    mac: data[i].toString().split(",")[1]
                };
                DHCPIps.push(entry);
            }   

            callback(DHCPIps);
        }
    });
}

function ReloadInterfaceAliasFile() 
{
    var aliasFileData = fs.readFileSync(NovaHomePath + "/config/interface_aliases.txt");
    interfaceAliases = JSON.parse(aliasFileData);
}

function ConvertInterfacesToAliases(interfaces) 
{
    var aliases = new Array();
    for (var i in interfaces) 
    {
        aliases.push(ConvertInterfaceToAlias(interfaces[i]));
    }
    return aliases;
}

function ConvertInterfaceToAlias(iface) 
{
    if (interfaceAliases[iface] !== undefined) 
    {
        return interfaceAliases[iface];
    } 
    else 
    {
        return iface;
    }
}

setInterval(function()
{
    try 
    {
        everyone.now.updateHaystackStatus(nova.IsHaystackUp());
        everyone.now.updateNovadStatus(nova.IsNovadUp(false));
    } 
    catch(err) 
    {

    }
}, 5000);
