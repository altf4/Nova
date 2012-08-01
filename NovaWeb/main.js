//"use strict";
var novaconfig = require('novaconfig.node');

var nova = new novaconfig.Instance();
var config = new novaconfig.NovaConfigBinding();
var honeydConfig = new novaconfig.HoneydConfigBinding();
var vendorToMacDb = new novaconfig.VendorMacDbBinding();
var osPersonalityDb = new novaconfig.OsPersonalityDbBinding();
var trainingDb = new novaconfig.CustomizeTrainingBinding();
var whitelistConfig = new novaconfig.WhitelistConfigurationBinding();
var hhconfig = new novaconfig.HoneydAutoConfigBinding();

honeydConfig.LoadAllTemplates();

var fs = require('fs');
var path = require('path');
var jade = require('jade');
var express =require('express');
var util = require('util');
var https = require('https');
var passport = require('passport');
var LocalStrategy = require('passport-local').Strategy;
var mysql = require('mysql');
var validCheck = require('validator').check;
var sanitizeCheck = require('validator').sanitize;
	
var exec = require('child_process').exec;


var Tail = require('tail').Tail;
var novadLog = new Tail("/usr/share/nova/Logs/Nova.log");


var credDb = 'nova_credentials';
var credTb = 'credentials';

var select;
var checkPass;
var my_name;

console.log("Starting NOVAWEB version " + config.GetVersionString());


// TODO: Get this path from the config class
process.chdir("/usr/share/nova/nova");

var client = mysql.createClient({
  user: 'root'
  , password: 'root'
  , database: credDb
});

passport.serializeUser(function(user, done) {
  done(null, user);
});

passport.deserializeUser(function(user, done) {
  done(null, user);
});

passport.use(new LocalStrategy(
  function(username, password, done) 
  {
    var user = username;
    process.nextTick(function()
    {
         client.query(
            'SELECT user, pass FROM ' + credTb + ' WHERE pass = SHA1(' + client.escape(password) + ')',
            function selectCb(err, results, fields, fn)
            {
              if(err) 
              {
                throw err;
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
            }
          );
    });
  }
));

// Setup TLS
var express_options = {
key:  fs.readFileSync('/usr/share/nova/NovaWeb/serverkey.pem'),
	  cert: fs.readFileSync('/usr/share/nova/NovaWeb/servercert.pem')
};

var app = express.createServer(express_options);

app.configure(function () {
		app.use(express.bodyParser());
		app.use(express.cookieParser());
		app.use(express.methodOverride());
		app.use(express.session({ secret: 'nova toor' }));
		app.use(passport.initialize());
		app.use(passport.session());
		app.use(app.router);
});

app.set('views', __dirname + '/views');

app.set('view options', {layout: false});

// Do the following for logging
//console.info("Logging to ./serverLog.log");
//var logFile = fs.createWriteStream('./serverLog.log', {flags: 'a'});
//app.use(express.logger({stream: logFile}));


app.use(express.static('/usr/share/nova/NovaWeb/www'));

var WEB_UI_PORT = config.ReadSetting("WEB_UI_PORT");
console.info("Listening on port " + WEB_UI_PORT);
app.listen(WEB_UI_PORT);
var nowjs = require("now");
var everyone = nowjs.initialize(app);


var initLogWatch = function () {
var novadLog = new Tail("/usr/share/nova/Logs/Nova.log");
novadLog.on("line", function(data) {
	try {
		everyone.now.newLogLine(data);
	} catch (err) {
	
	}
});

novadLog.on("error", function(data) {
	console.log("ERROR: " + data);
	try {everyone.now.newLogLine(data)} 
	catch (err) 
	{
	
	}
});
}

initLogWatch();

function callScan(numNodes, subnets)
{
  hhconfig.RunAutoScan(numNodes, subnets);
}

app.get('/downloadNovadLog', ensureAuthenticated, function (req, res) {
	fs.readFile('/usr/share/nova/Logs/Nova.log', 'utf8', function (err,data) {
	  if (err) {
		res.send(err);
	  }
	  else
	  {
		var reply = data.toString().replace(/(\r\n|\n|\r)/gm,"<br>");
		res.send(reply);
	  }
	});
});

app.get('/advancedOptions', ensureAuthenticated, function(req, res) {
    var all = config.ListInterfaces().sort();
    var used = config.GetInterfaces().sort();
    
    var pass = [];
    
    for(var i in all)
    {
      var checked = false;
      
      for(var j in used)
      {
        if(all[i] === used[j])
        {
          checked = true;
          break;
        }
      }
      
      if(checked)
      {
        pass.push( { iface: all[i], checked: 1 } );
      }
      else
      {
        pass.push( { iface: all[i], checked: 0 } );
      }
    }
    
     res.render('advancedOptions.jade', 
	 {
		locals: {
      INTERFACES: config.ListInterfaces().sort() 
      ,DEFAULT: config.GetUseAllInterfacesBinding() 
			,HS_HONEYD_CONFIG: config.ReadSetting("HS_HONEYD_CONFIG")
			,TCP_TIMEOUT: config.ReadSetting("TCP_TIMEOUT")
			,TCP_CHECK_FREQ: config.ReadSetting("TCP_CHECK_FREQ")
			,READ_PCAP: config.ReadSetting("READ_PCAP")
			,PCAP_FILE: config.ReadSetting("PCAP_FILE")
			,GO_TO_LIVE: config.ReadSetting("GO_TO_LIVE")
			,CLASSIFICATION_TIMEOUT: config.ReadSetting("CLASSIFICATION_TIMEOUT")
			,SILENT_ALARM_PORT: config.ReadSetting("SILENT_ALARM_PORT")
			,K: config.ReadSetting("K")
			,EPS: config.ReadSetting("EPS")
			,IS_TRAINING: config.ReadSetting("IS_TRAINING")
			,CLASSIFICATION_THRESHOLD: config.ReadSetting("CLASSIFICATION_THRESHOLD")
			,DATAFILE: config.ReadSetting("DATAFILE")
			,SA_MAX_ATTEMPTS: config.ReadSetting("SA_MAX_ATTEMPTS")
			,SA_SLEEP_DURATION: config.ReadSetting("SA_SLEEP_DURATION")
			,USER_HONEYD_CONFIG: config.ReadSetting("USER_HONEYD_CONFIG")
			,DOPPELGANGER_IP: config.ReadSetting("DOPPELGANGER_IP")
			,DOPPELGANGER_INTERFACE: config.ReadSetting("DOPPELGANGER_INTERFACE")
			,DM_ENABLED: config.ReadSetting("DM_ENABLED")
			,ENABLED_FEATURES: config.ReadSetting("ENABLED_FEATURES")
			,FEATURE_NAMES: nova.GetFeatureNames()
			,TRAINING_CAP_FOLDER: config.ReadSetting("TRAINING_CAP_FOLDER")
			,THINNING_DISTANCE: config.ReadSetting("THINNING_DISTANCE")
			,SAVE_FREQUENCY: config.ReadSetting("SAVE_FREQUENCY")
			,DATA_TTL: config.ReadSetting("DATA_TTL")
			,CE_SAVE_FILE: config.ReadSetting("CE_SAVE_FILE")
			,SMTP_ADDR: config.ReadSetting("SMTP_ADDR")
			,SMTP_PORT: config.ReadSetting("SMTP_PORT")
			,SMTP_DOMAIN: config.ReadSetting("SMTP_DOMAIN")
			,RECIPIENTS: config.ReadSetting("RECIPIENTS")
			,SERVICE_PREFERENCES: config.ReadSetting("SERVICE_PREFERENCES")
			,HAYSTACK_STORAGE: config.ReadSetting("HAYSTACK_STORAGE")
		}
	 });
});


function renderBasicOptions(jadefile, res, req) {
    var all = config.ListInterfaces().sort();
    var used = config.GetInterfaces().sort();
    
    var pass = [];
    
    for(var i in all)
    {
      var checked = false;
      
      for(var j in used)
      {
        if(all[i] === used[j])
        {
          checked = true;
          break;
        }
      }
      
      if(checked)
      {
        pass.push( { iface: all[i], checked: 1 } );
      }
      else
      {
        pass.push( { iface: all[i], checked: 0 } );
      }
    }
    
     res.render(jadefile, 
	 { 
		locals: {
            INTERFACES: pass 
            ,DEFAULT: config.GetUseAllInterfacesBinding() 
			,DOPPELGANGER_IP: config.ReadSetting("DOPPELGANGER_IP")
			,DOPPELGANGER_INTERFACE: config.ReadSetting("DOPPELGANGER_INTERFACE")
			,DM_ENABLED: config.ReadSetting("DM_ENABLED")
			,SMTP_ADDR: config.ReadSetting("SMTP_ADDR")
			,SMTP_PORT: config.ReadSetting("SMTP_PORT")
			,SMTP_DOMAIN: config.ReadSetting("SMTP_DOMAIN")
			,RECIPIENTS: config.ReadSetting("RECIPIENTS")
		}
	 });
}

app.get('/error', ensureAuthenticated, function(req, res) {
	
	console.log(req.query);
	
	res.render('error.jade', { locals: { redirectLink: req.query["redirectLink"], errorDetails: req.query["errorDetails"] }});
});

app.get('/basicOptions', ensureAuthenticated, function(req, res) {
	renderBasicOptions('basicOptions.jade', res, req);
});

app.get('/configHoneydNodes', ensureAuthenticated, function(req, res) {
	honeydConfig.LoadAllTemplates();
	 var nodeNames = honeydConfig.GetNodeNames();
	 var nodes = new Array();
	 
	 for (var i = 0; i < nodeNames.length; i++) {
		nodes.push(honeydConfig.GetNode(nodeNames[i]));
	 }
     
	 res.render('configHoneyd.jade', 
	 { locals: {
	 	INTERFACES: config.ListInterfaces().sort()
	 	,profiles: honeydConfig.GetProfileNames()
	 	,nodes: nodes
		,subnets:  honeydConfig.GetSubnetNames()
		,groups: honeydConfig.GetGroups()
		,currentGroup: config.GetGroup()
	}})
});


app.get('/configHoneydProfiles', ensureAuthenticated, function(req, res) {
	honeydConfig.LoadAllTemplates();
	 var profileNames = honeydConfig.GetProfileNames();
	 var profiles = [];
	 for (var i = 0; i < profileNames.length; i++) {
		profiles.push(honeydConfig.GetProfile(profileNames[i]));
	 }
     
	 res.render('configHoneydProfiles.jade', 
	 { locals: {
	 	profileNames: honeydConfig.GetProfileNames()
	 	,profiles: profiles
	}})
});

app.get('/editHoneydNode', ensureAuthenticated, function(req, res) {
	nodeName = req.query["node"];
	// TODO: Error checking for bad node names
	
	node = honeydConfig.GetNode(nodeName); 
	res.render('editHoneydNode.jade', 
	{ locals : {
		oldName: nodeName
		, INTERFACES: config.ListInterfaces().sort()
	 	, profiles: honeydConfig.GetProfileNames()
		, profile: node.GetProfile()
		, ip: node.GetIP()
		, mac: node.GetMAC()
	}})
});

app.get('/editHoneydProfile', ensureAuthenticated, function(req, res) {
	profileName = req.query["profile"]; 

	res.render('editHoneydProfile.jade', 
	{ locals : {
		oldName: profileName
		, parentName: ""
		, newProfile: false
		, vendors: vendorToMacDb.GetVendorNames()
		, scripts: honeydConfig.GetScriptNames()
		, personalities: osPersonalityDb.GetPersonalityOptions()
	}})
});


app.get('/addHoneydProfile', ensureAuthenticated, function(req, res) {
	parentName = req.query["parent"]; 

	res.render('addHoneydProfile.jade', 
	{ locals : {
		oldName: parentName
		, parentName: parentName
		, newProfile: true
		, vendors: vendorToMacDb.GetVendorNames()
		, scripts: honeydConfig.GetScriptNames()
		, personalities: osPersonalityDb.GetPersonalityOptions()
	}})
});

app.get('/customizeTraining', ensureAuthenticated, function(req, res) {
	trainingDb = new novaconfig.CustomizeTrainingBinding();
	res.render('customizeTraining.jade',
	{ locals : {
		desc: trainingDb.GetDescriptions()
		, uids: trainingDb.GetUIDs()
		, hostiles: trainingDb.GetHostile()	
	}})
});

app.get('/importCapture', ensureAuthenticated, function(req, res) {
	var trainingSession = req.query["trainingSession"];
	trainingSession = "/usr/share/nova/nova/Data/" + trainingSession + "/capture.dump";
	var ips = trainingDb.GetCaptureIPs(trainingSession);

	if (ips == undefined) {
	  	res.render('error.jade', { locals: { redirectLink: "/", errorDetails: "Unable to read capture dump file" }});
	} else {
		res.render('importCapture.jade',
		{ locals : {
			ips: trainingDb.GetCaptureIPs(trainingSession),
			trainingSession: req.query["trainingSession"] 
		}})
	}
});

app.post('/changeGroup', ensureAuthenticated, function(req, res){
	var selectedGroup = req.body["GROUP"];
	
	config.SetGroup(selectedGroup);
	
	res.redirect('/configHoneydNodes');
});

app.post('/importCaptureSave', ensureAuthenticated, function(req, res) {	
	var hostileSuspects = new Array();
  	var includedSuspects = new Array();
	var descriptions = new Object();
	
	var trainingSession = req.query["trainingSession"];
	trainingSession = "/usr/share/nova/nova/Data/" + trainingSession + "/capture.dump";
	
	var trainingDump = new novaconfig.TrainingDumpBinding();
	if (!trainingDump.LoadCaptureFile(trainingSession)) {
	  	res.render('error.jade', { locals: { redirectLink: "/", errorDetails: "Unable to parse dump file " + trainingSession }});
	}

	trainingDump.SetAllIsIncluded(false);
	trainingDump.SetAllIsHostile(false);

  	for(var id in req.body) {
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
	if (!trainingDump.SaveToDb("/usr/share/nova/nova/Config/training.db")) {
	  	res.render('error.jade', { locals: { redirectLink: "/", errorDetails: "Unable to save to training db"}});
	}

	res.render('saveRedirect.jade', { locals: {redirectLink: "/customizeTraining"}})	

});

app.get('/configWhitelist', ensureAuthenticated, function(req, res) {
	res.render('configWhitelist.jade',
	{ locals : {
		whitelistedIps: whitelistConfig.GetIps(),
		whitelistedRanges: whitelistConfig.GetIpRanges()
	}})
});

app.get('/editUsers', ensureAuthenticated, function(req, res) {
	var usernames = new Array();
  client.query(
    'SELECT user FROM ' + credTb,
    function (err, results, fields) {
      if(err) {
        throw err;
      }

	var usernames = new Array();
	for (var i in results) {
		usernames.push(results[i].user);
	}
	res.render('editUsers.jade',
	{ locals: {
		usernames: usernames
	}});
    } 
  );
});

app.get('/configWhitelist', ensureAuthenticated, function(req, res) {
	res.render('configWhitelist.jade',
	{ locals : {
		whitelistedIps: whitelistConfig.GetIps(),
		whitelistedRanges: whitelistConfig.GetIpRanges()
	}})
});

app.get('/editUsers', ensureAuthenticated, function(req, res) {
	var usernames = new Array();
  client.query(
    'SELECT user FROM ' + credTb,
    function (err, results, fields) {
      if(err) {
        throw err;
      }

	var usernames = new Array();
	for (var i in results) {
		usernames.push(results[i].user);
	}
	res.render('editUsers.jade',
	{ locals: {
		usernames: usernames
	}});
    } 
  );
});

app.get('/logout', function(req, res){
  req.logout();
  res.redirect('/');
});

app.get('/suspects', ensureAuthenticated, function(req, res) {
	 var type;
	 if (req.query["type"] == undefined) {
	   type = 'all'; 
	 } else {
       type = req.query["type"];
	 }

     res.render('main.jade', 
     {
         user: req.user
	     , enabledFeatures: config.ReadSetting("ENABLED_FEATURES")
		 , featureNames: nova.GetFeatureNames()
		 , type: type

     });
});

app.get('/novadlog', ensureAuthenticated, function(req, res) {
	initLogWatch();
	res.render('novadlog.jade');
});

app.get('/', ensureAuthenticated, function(req, res) {res.redirect('/suspects');});
app.get('/createNewUser', ensureAuthenticated, function(req, res) {res.render('createNewUser.jade');});
app.get('/welcome', ensureAuthenticated, function(req, res) {res.render('welcome.jade');});
app.get('/setup1', ensureAuthenticated, function(req, res) {res.render('setup1.jade');});
app.get('/setup2', ensureAuthenticated, function(req, res) {renderBasicOptions('setup2.jade', res, req)});
app.get('/setup3', ensureAuthenticated, function(req, res) {res.render('setup3.jade');});
app.get('/CaptureTrainingData', ensureAuthenticated, function(req, res) {res.render('captureTrainingData.jade');});
app.get('/about', ensureAuthenticated, function(req, res) {res.render('about.jade');});

app.post('/createNewUser', ensureAuthenticated, function(req, res) {
	var password = req.body["password"];
	var userName = req.body["username"];
	
    client.query('SELECT user FROM ' + credTb + ' WHERE user = ' + client.escape(userName) + '',
    function selectCb(err, results, fields) {
      if(err) {
	  	res.render('error.jade', { locals: { redirectLink: "/createNewUser", errorDetails: "Unable to access authentication database" }});
		return;
      }

      if(results[0] == undefined)
      {
	    
        client.query('INSERT INTO ' + credTb + ' values(' + client.escape(userName) + ', SHA1(' + client.escape(password) + '))');
		res.render('saveRedirect.jade', { locals: {redirectLink: "/"}})	
        return;
      } 
      else
      {
	  	res.render('error.jade', { locals: { redirectLink: "/createNewUser", errorDetails: "Username you entered already exists. Please choose another." }});
		return;
      }
	  });
});


app.post('/createInitialUser', ensureAuthenticated, function(req, res) {
	var password = req.body["password"];
	var userName = req.body["username"];
	
    client.query('SELECT user FROM ' + credTb + ' WHERE user = ' + client.escape(userName) + '',
    function selectCb(err, results, fields) {
      if(err) {
	  	res.render('error.jade', { locals: { redirectLink: "/createNewUser", errorDetails: "Unable to access authentication database" }});
		return;
      }

      if(results[0] == undefined)
      {
	    
        client.query('INSERT INTO ' + credTb + ' values(' + client.escape(userName) + ', SHA1(' + client.escape(password) + '))');
		client.query('DELETE FROM ' + credTb + ' WHERE user = ' + client.escape('nova'));
		res.render('saveRedirect.jade', { locals: {redirectLink: "setup2"}})	
        return;
      } 
      else
      {
	  	res.render('error.jade', { locals: { redirectLink: "/setup1", errorDetails: "Username you entered already exists. Please choose another." }});
		return;
      }
	  });
});

app.get('/login', function(req, res){
	 var redirect;
	 if (req.query["redirect"] != undefined)
	 {
		redirect = req.query["redirect"]; 
	 } else {
		redirect = "/suspects";
	 }
     res.render('login.jade',
     {
         user: req.user
         , message: req.flash('error')  
		 , redirect: redirect
     });

});

app.get('/autoConfig', ensureAuthenticated, function(req, res) {
   res.render('hhautoconfig.jade', 
   {
     user: req.user
     ,INTERFACES: config.ListInterfaces().sort()
   });
});

app.get('/nodeReview', ensureAuthenticated, function(req, res) { 
	honeydConfig.LoadAllTemplates();
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
     
	 res.render('nodereview.jade', 
	 { locals: {
	 	profileNames: honeydConfig.GetGeneratedProfileNames()
	 	,profiles: profiles
	 	,nodes: nodes
	 	,subnets:  honeydConfig.GetSubnetNames() 
	}})
});

app.post('/login*',
  passport.authenticate('local', { failureRedirect: '/', failureFlash: true }), 
    function(req, res){
		if(req.query["redirect"] != undefined)
		{
        	res.redirect(req.query["redirect"]);
		}
		else
		{
        	res.redirect('/suspects');
		}
});

app.post('/scanning', ensureAuthenticated, function(req, res){
  
  var numNodes = req.body["numNodes"];
  
  var subnets = req.body["subnets"];
  
  var interfaces = req.body["interfaces"];
  
  if(interfaces === undefined)
  {
    interfaces = "";
    console.log("No interfaces selected.");
  }
  
  if(subnets === undefined)
  {
    subnets = "";
    console.log("No additional subnets selected.");
  }
  
  if((subnets === "" && interfaces === "") && (subnets === undefined && interfaces === undefined))
  {
    res.redirect('/autoConfig');
  }
  else
  {
    hhconfig.RunAutoScan(numNodes, interfaces, subnets);
  
    res.redirect('/nodeReview'); 
  }
});

app.post('/customizeTrainingSave', ensureAuthenticated, function(req, res){
  for(var uid in req.body)
  {
      trainingDb.SetIncluded(uid, true);
  }
	
	trainingDb.Save();
	
	res.render('saveRedirect.jade', { locals: {redirectLink: "/customizeTraining"}})	
});

app.post('/editHoneydNodesSave', ensureAuthenticated, function(req, res) {
	var ipAddress;
	if (req.body["ipType"] == "DHCP") {
		ipAddress = "DHCP";
	} else{
		ipAddress = req.body["ip1"] + "." + req.body["ip2"] + "." + req.body["ip3"] + "." + req.body["ip4"];
	}

	var profile = req.body["profile"];
	var intface = req.body["interface"];
	var subnet = "";
	var count = Number(req.body["nodeCount"]);

	console.log("Creating new nodes:" + profile + " " + ipAddress + " " + intface + " " + count);
	honeydConfig.AddNewNodes(profile, ipAddress, intface, subnet, count);
	honeydConfig.SaveAllTemplates();
     
	res.render('saveRedirect.jade', { locals: {redirectLink: "/configHoneydNodes"}})

});

app.post('/editHoneydNodeSave', ensureAuthenticated, function(req, res) {
	var profile = req.body["profile"];
	var intface = req.body["interface"];
	var oldName = req.body["oldName"];
	var subnet = "";
	
	var ipAddress;
	if (req.body["ipType"] == "DHCP") {
		ipAddress = "DHCP";
	} else {
		ipAddress = req.body["ip0"] + "." +  req.body["ip1"] + "." + req.body["ip2"] + "." + req.body["ip3"];
	}
	
	var macAddress;	
	if (req.body["macType"] == "RANDOM") {
		macAddress = "RANDOM";
	} else {
		macAddress = req.body["mac0"] + ":" + req.body["mac1"] + ":" + req.body["mac2"] + ":" + req.body["mac3"] + ":" + req.body["mac4"] + ":" + req.body["mac5"];
	}
    // Delete the old node and then add the new one	
	honeydConfig.DeleteNode(oldName);
	if(!honeydConfig.AddNewNode(profile, ipAddress, macAddress, intface, subnet))
	{
	  res.render('error.jade', { locals: { redirectLink: "/configHoneydNodes", errorDetails: "AddNewNode failed" }});
	}
	else
	{
	  honeydConfig.SaveAllTemplates();
	  res.render('saveRedirect.jade', { locals: {redirectLink: "/configHoneydNodes"}})
	}
});

app.post('/configureNovaSave', ensureAuthenticated, function(req, res) {
	// TODO: Throw this out and do error checking in the Config (WriteSetting) class instead
	var configItems = ["DEFAULT", "INTERFACE", "HS_HONEYD_CONFIG","TCP_TIMEOUT","TCP_CHECK_FREQ","READ_PCAP","PCAP_FILE",
		"GO_TO_LIVE","CLASSIFICATION_TIMEOUT","SILENT_ALARM_PORT","K","EPS","IS_TRAINING","CLASSIFICATION_THRESHOLD","DATAFILE",
		"SA_MAX_ATTEMPTS","SA_SLEEP_DURATION","USER_HONEYD_CONFIG","DOPPELGANGER_IP","DOPPELGANGER_INTERFACE","DM_ENABLED",
		"ENABLED_FEATURES","TRAINING_CAP_FOLDER","THINNING_DISTANCE","SAVE_FREQUENCY","DATA_TTL","CE_SAVE_FILE","SMTP_ADDR",
		"SMTP_PORT","SMTP_DOMAIN","RECIPIENTS","SERVICE_PREFERENCES","HAYSTACK_STORAGE"];
  
  var Validator = require('validator').Validator;
  
  //Function overrides for error functionality in the Validator class
  Validator.prototype.error = function(msg)
  {
    this._errors.push(msg);
  }
  
  Validator.prototype.getErrors = function()
  {
    return this._errors;
  }
  
  var validator = new Validator();
  
  config.ClearInterfaces();
  
  var interfaces = "";
  var oneIface = false;
  
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
  
    if(oneIface)
    {
      config.AddIface(interfaces);
    }
    
    req.body["INTERFACE"] = interfaces;
  }
  
  for(var item = 0; item < configItems.length; item++)
  {
    if (req.body[configItems[item]] == undefined) {
		continue;
	  }
    switch(configItems[item])
    {
      case "SA_SLEEP_DURATION":
        validator.check(req.body[configItems[item]], 'Must be a nonnegative integer or floating point number').isFloat();
        break;
        
      case "TCP_TIMEOUT":
        validator.check(req.body[configItems[item]], 'Must be a nonnegative integer').isInt();
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
        
        if(split.length == 4)
        {
          if(split[3] === "0")
          {
            validator.check(split[3], 'Can not have last IP octet be 0').equals("255");
          }
          if(split[3] === "255")
          {
            validator.check(split[3], 'Can not have last IP octet be 255').equals("0");
          }
        }
        
        //check for 0.0.0.0 and 255.255.255.255
        var checkIPZero = 0;
        var checkIPBroad = 0;
        
        for(var val = 0; val < split.length; val++)
        {
          if(split[val] == "0")
          {
            checkIPZero++;
          }
          if(split[val] == "255")
          {
            checkIPBroad++;
          } 
        }
        
        if(checkIPZero == 4)
        {
          validator.check(checkIPZero, '0.0.0.0 is not a valid IP address').is("200");
        }
        if(checkIPBroad == 4)
        {
          validator.check(checkIPZero, '255.255.255.255 is not a valid IP address').is("200");
        }
        break;
     
      case "SMTP_ADDR":
        validator.check(req.body[configItems[item]], 'SMTP Address is the wrong format').regex('^(([A-z]|[0-9])*\\.)*(([A-z]|[0-9])*)\\@((([A-z]|[0-9])*)\\.)*(([A-z]|[0-9])*)\\.(([A-z]|[0-9])*)$');
        break;
      
      case "SMTP_DOMAIN":
        validator.check(req.body[configItems[item]], 'SMTP Domain is the wrong format').regex('^(([A-z]|[0-9])+\\.)+(([A-z]|[0-9])+)$');
        break; 
        
      case "RECIPIENTS":
        validator.check(req.body[configItems[item]], 'Recipients list should be a comma separated list of email addresses').regex('^(([A-z]|\d)+(\\.([A-z]|\d)+)*)\\@(([A-z]|\d)+(\\.([A-z]|\d)+)+)((\\,){1}( )?(([A-z]|\d)+(\\.([A-z]|\d)+)*)\\@(([A-z]|\d)+(\\.([A-z]|\d)+)+))*$');
        break;
        
      case "SERVICE_PREFERENCES":
        validator.check(req.body[configItems[item]], "Service Preferences string is formatted incorrectly").is('^0:[0-7](\\+|\\-)?;1:[0-7](\\+|\\-)?;2:[0-7](\\+|\\-)?;$');
        break;
        
      default:
        break;
    }
  }
  
  
  var errors = validator.getErrors();
  
  if(errors.length > 0)
  {
    res.render('error.jade', { locals: {errorDetails: errors, redirectLink: "/basicOptions"} });
  }
  else
  {
    if(req.body["INTERFACE"] !== undefined && req.body["DEFAULT"] === undefined)
    {
      req.body["DEFAULT"] = false;
      config.UseAllInterfaces(false);
      config.WriteSetting("INTERFACE", req.body["INTERFACE"]);
    }
    else if(req.body["INTERFACE"] === undefined)
    {
      req.body["DEFAULT"] = true;
      config.UseAllInterfaces(true);
      config.WriteSetting("INTERFACE", "default");
    }
    else
    {  
      config.UseAllInterfaces(req.body["DEFAULT"]);
      config.WriteSetting("INTERFACE", req.body["INTERFACE"]);
    }

    //if no errors, send the validated form data to the WriteSetting method
    for(var item = 2; item < configItems.length; item++)
    {
  	  if(req.body[configItems[item]] != undefined) 
  	  {
        	config.WriteSetting(configItems[item], req.body[configItems[item]]);  
  	  }
    }
    
    res.render('saveRedirect.jade', { locals: {redirectLink: "/suspects"}}) 
  }
});

// Functions to be called by clients
everyone.now.ClearAllSuspects = function(callback)
{
	console.log("Attempting to clear all suspects in main.js");

	nova.CheckConnection();
	nova.ClearAllSuspects();
}

everyone.now.GetInheritedEthernetList = function(parent, callback)
{
	var prof = honeydConfig.GetProfile(parent);
	
	callback(prof.GetVendors(), prof.GetVendorDistributions());
}

everyone.now.StartHaystack = function()
{
	if (!nova.IsHaystackUp()) {
		nova.StartHaystack();
	}
}

everyone.now.StopHaystack = function()
{
	nova.StopHaystack();
}


everyone.now.IsNovadUp = function()
{
	return nova.IsNovadIp();
}

everyone.now.StartNovad = function()
{
	nova.StartNovad();
	nova.CheckConnection();
}

everyone.now.StopNovad = function()
{
	nova.StopNovad();
	nova.CloseNovadConnection();
}


everyone.now.sendAllSuspects = function(callback)
{
	nova.getSuspectList(callback);
}


everyone.now.deleteUserEntry = function(usernamesToDelete, callback)
{
	var username;
	for (var i = 0; i < usernamesToDelete.length; i++) {
		username = String(usernamesToDelete[i]);
		var query = 'DELETE FROM ' + credTb + ' WHERE user = ' + client.escape(username);
 		client.query(query);
	}

	// TODO: Error handling? Bit of a pain async. could change to single SQL query and 
	// put the callback in the callback for .query.
	callback(true);
}

everyone.now.updateUserPassword = function (username, newPassword, callback) {
	var query = 'UPDATE ' + credTb + ' SET pass = SHA1(' + client.escape(String(newPassword)) + ') WHERE user = ' + client.escape(String(username));
 	client.query(query,
    function selectCb(err, results, fields) {
    	if(err) {
        	callback(false, "Unable to access user database: " + err);
			return;
      	}
		callback(true);
	});	
}

// Deletes a honeyd node
everyone.now.deleteNodes = function(nodeNames, callback)
{
	var nodeName;
	for (var i = 0; i < nodeNames.length; i++) {
		nodeName = nodeNames[i];
	
		console.log("Deleting honeyd node " + nodeName);

		if (!honeydConfig.DeleteNode(nodeName)) {
			callback(false, "Failed to delete node " + nodeName);
			return;
		}

		if (!honeydConfig.SaveAllTemplates())
		{
			callback(false, "Failed to save XML templates");
			return;
		}
	}
	
	callback(true, "");
}

everyone.now.deleteProfiles = function(profileNames, callback)
{
	var profileName;
	for (var i = 0; i < profileNames.length; i++) {
		profileName = profileNames[i];
	
		if (!honeydConfig.DeleteProfile(profileName)) {
			callback(false, "Failed to delete profile " + profileName);
			return;
		}
	
	
		if (!honeydConfig.SaveAllTemplates()) {
			callback(false, "Failed to save XML templates");
			return;
		}
	}

	callback(true, "");
}

everyone.now.addWhitelistEntry = function(entry, callback)
{

	// TODO: Input validation. Should be IP address or 'IP/netmask'
	// Should also be sanitized for newlines/trailing whitespace
	if (whitelistConfig.AddEntry(entry))
	{
		callback(true, "");
	}
	else
	{
		callback(true, "Attempt to add whitelist entry failed");
	}
}

everyone.now.deleteWhitelistEntry = function(whitelistEntryNames, callback)
{
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

everyone.now.GetProfile = function(profileName, callback) {
	console.log("Fetching profile " + profileName);
	var profile = honeydConfig.GetProfile(profileName);
	
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

    if(!profile.isEthernetInherited)
    {
      var ethVendorList = [];
    
      var profVendors = profile.GetVendors();
      var profDists = profile.GetVendorDistributions();

      for(var i = 0; i < profVendors.length; i++)
      {
        var element = {vendor: "", dist: ""};
        element.vendor = profVendors[i];
        element.dist = parseFloat(profDists[i]);
        ethVendorList.push(element);
      }
      
      profile.ethernet = ethVendorList;
    }

    callback(profile);
}

everyone.now.GetVendors = function(profileName, callback)
{
	var profile = honeydConfig.GetProfile(profileName);
	
	var ethVendorList = [];
    
    var profVendors = profile.GetVendors();
    var profDists = profile.GetVendorDistributions();

    for(var i = 0; i < profVendors.length; i++)
    {
      var element = {vendor: "", dist: ""};
      element.vendor = profVendors[i];
      element.dist = parseFloat(profDists[i]);
      ethVendorList.push(element);
    }
	
    callback(profVendors, profDists);
}

everyone.now.GetPorts = function (profileName, callback) {
    var ports = honeydConfig.GetPorts(profileName);
    
    for ( var i = 0; i < ports.length; i++) {
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


everyone.now.SaveProfile = function(profile, ports, callback, ethVendorList, addOrEdit) {
        var honeydProfile = new novaconfig.HoneydProfileBinding();

        console.log("Got profile " + profile.name + "_" + profile.personality);
        console.log("Got portlist " + ports.name);
        console.log("Got ethVendorList " + ethVendorList);

        // Move the Javascript object values to the C++ object
        honeydProfile.SetName(profile.name);
        honeydProfile.SetTcpAction(profile.tcpAction);
        honeydProfile.SetUdpAction(profile.udpAction);
        honeydProfile.SetIcmpAction(profile.icmpAction);
        honeydProfile.SetPersonality(profile.personality);

        if(ethVendorList == undefined || ethVendorList == null)
        {
            honeydProfile.SetEthernet(profile.ethernet);
        }
        else if(profile.isEthernetInherited == false)
        {
            var ethVendors = [];
            var ethDists = [];
        
            for(var i = 0; i < ethVendorList.length; i++)
            {
                ethVendors.push(ethVendorList[i].vendor);
                ethDists.push(parseFloat(ethVendorList[i].dist));
            }
            
            honeydProfile.SetVendors(ethVendors, ethDists);
        }

        honeydProfile.SetUptimeMin(profile.uptimeMin);
        honeydProfile.SetUptimeMax(profile.uptimeMax);
        honeydProfile.SetDropRate(profile.dropRate);
        honeydProfile.SetParentProfile(profile.parentProfile);
        
        honeydProfile.setTcpActionInherited(profile.isTcpActionInherited);
        honeydProfile.setUdpActionInherited(profile.isUdpActionInherited);
        honeydProfile.setIcmpActionInherited(profile.isIcmpActionInherited);
        honeydProfile.setPersonalityInherited(profile.isPersonalityInherited);
        honeydProfile.setEthernetInherited(profile.isEthernetInherited);
        honeydProfile.setUptimeInherited(profile.isUptimeInherited);
        honeydProfile.setDropRateInherited(profile.isDropRateInherited);


        // Add new ports
        var portName;
        for (var i = 0; i < ports.size; i++) {
	        console.log("Adding port " + ports[i].portNum + " " + ports[i].type + " " + ports[i].behavior + " " + ports[i].script + " Inheritance: " + ports[i].isInherited);
	        portName = honeydConfig.AddPort(Number(ports[i].portNum), Number(ports[i].type), Number(ports[i].behavior), ports[i].script);

	        if (portName != "") {
		        honeydProfile.AddPort(portName, ports[i].isInherited);
	        }
	    }

        var portTest = honeydProfile.GetPortNames();

        for(var i = 0; i < portTest.length; i++)
        {
            console.log("Port Name " + (i + 1) + ": " + portTest[i]);
        }

        honeydConfig.SaveAllTemplates();

        // Save the profile
        honeydProfile.Save();

        callback();
}

everyone.now.GetCaptureSession = function (callback) {
	var ret = config.ReadSetting("TRAINING_SESSION");
	callback(ret);
}

everyone.now.StartTrainingCapture = function (trainingSession, callback) {
	config.WriteSetting("IS_TRAINING", "1");
	config.WriteSetting("TRAINING_SESSION", trainingSession.toString());

	// Check if training folder already exists
	//console.log(Object.keys(fs));
	path.exists("/usr/share/nova/nova/Data/" + trainingSession, function(exists) {
		if (exists) {
			callback("Training session folder already exists for session name of '" + trainingSession + "'");
			return;
		} else {
			// Start the haystack
			if (!nova.IsHaystackUp()) {
				nova.StartHaystack();
			}

			// (Re)start NOVA
			nova.StopNovad();
			nova.StartNovad();

			nova.CheckConnection();

			callback();
		}
	});
}

everyone.now.StopTrainingCapture = function (trainingSession, callback) {
	config.WriteSetting("IS_TRAINING", "0");
	config.WriteSetting("TRAINING_SESSION", "null");
	nova.StopNovad();

	exec('novatrainer /usr/share/nova/nova/Data/' + trainingSession + ' /usr/share/nova/nova/Data/' + trainingSession + '/capture.dump', 
	function (error, stdout, stderr) {
		callback(stderr);
	});
}

everyone.now.GetCaptureIPs = function (trainingSession, callback) {
	return trainingDb.GetCaptureIPs("/usr/share/nova/nova/Data/" + trainingSession + "/capture.dump");
}

var distributeSuspect = function(suspect)
{
	var s = new Object();
	objCopy(suspect, s);
	everyone.now.OnNewSuspect(s)
};

var distributeAllSuspectsCleared = function()
{
	console.log("Distribute all suspects cleared called in main.js");
	everyone.now.AllSuspectsCleared();
}

nova.registerOnAllSuspectsCleared(distributeAllSuspectsCleared);
nova.registerOnNewSuspect(distributeSuspect);


process.on('SIGINT', function() {
  client.destroy();
	nova.Shutdown();
	process.exit();
});

function objCopy(src,dst) {
	for (var member in src)
	{
		if( typeof src[member] == 'function' )
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

function ensureAuthenticated(req, res, next) {
  if (req.isAuthenticated()) {
    if (req.url == '/' || req.url == '/suspects') {
        client.query('SELECT user FROM ' + credTb + ' WHERE user = ' + client.escape('nova'),
        function selectCb(err, results, fields) {
            if(!err) {
                if (results.length >= 1) {
                    res.redirect('/welcome');
                } else {
                    return next();
                }
            }
        });
    } else {
        return next();
    }
      
  } else {
	 if (req.url != "/login")
	 {
  		res.redirect("/login?redirect=" + req.url);
	 } else {
  		res.redirect('/login');
	 }
  }
}

function queryCredDb(check) {
    client.query(
    'SELECT pass FROM ' + credTb + ' WHERE pass = SHA1(' + client.escape(check) + ')',
    function selectCb(err, results, fields) {
      if(err) {
        throw err;
      }
      
      select = results[0].pass;      
      
      if(select === results[0].pass)
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  );
};

function switcher(err, user, success, done)
{
   if(!success) { return done(null, false, { message: 'Username/password combination is incorrect' }); }
   my_name = user;
   return done(null, user);
}

setInterval(function() {
		try {
			everyone.now.updateHaystackStatus(nova.IsHaystackUp());
			everyone.now.updateNovadStatus(nova.IsNovadUp(false));
		} catch (err)
		{

		}
}, 5000);
