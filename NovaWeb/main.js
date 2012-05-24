var novaconfig = require('novaconfig.node');

var nova = new novaconfig.Instance();
var config = new novaconfig.NovaConfigBinding();
var honeydConfig = new novaconfig.HoneydConfigBinding();
var vendorToMacDb = new novaconfig.VendorMacDbBinding();
var osPersonalityDb = new novaconfig.OsPersonalityDbBinding();
var trainingDb = new novaconfig.CustomizeTrainingBinding();
var whitelistConfig = new novaconfig.WhitelistConfigurationBinding();

honeydConfig.LoadAllTemplates();

var fs = require('fs');
var jade = require('jade');
var express =require('express');
var util = require('util');
var https = require('https');
var passport = require('passport');
var LocalStrategy = require('passport-local').Strategy;
var mysql = require('mysql');
var validCheck = require('validator').check;
var sanitizeCheck = require('validator').sanitize;

var credDb = 'nova_credentials';
var credTb = 'credentials'

var select;
var checkPass;
var my_name;

// TODO: Get this path from the config class
process.chdir("/usr/share/nova/nova");

var client = mysql.createClient({
  user: 'root'
  , password: 'root'
});

client.useDatabase(credDb);

var tempUser = [ {id: 1, username: 'nova', password: 'toor'} ];

function findById(id, fn) {
  var index = id - 1;
  if(tempUser[index])
  {
    fn(null, tempUser[index]);
  }
  else
  {
    fn(new Error('User ' + id + ' does not exist'));
  }
}

function findByUsername(username, fn) {
  for(var i = 0, len = tempUser.length; i < len; i++)
  {
    if(username === my_name)
    {
      return fn(null, username);
    }
  }  
  return fn(null, null);
}

passport.serializeUser(function(user, done) {
  done(null, user);
});

passport.deserializeUser(function(user, done) {
  findByUsername(user, function(err, user) {
    done(err, user)
  });
});

passport.use(new LocalStrategy(
  function(username, password, done) 
  {
    process.nextTick(function()
    {
      checkUsername(username, function(err, user)
      { 
         if(err) { return done(err); }
         if(!user) { return done(null, false, { message: 'Unknown user ' + username }); }
         client.query(
            'SELECT user, pass FROM ' + credTb + ' WHERE pass = PASSWORD(\'' + password + '\')',
            function selectCb(err, results, fields, fn) 
            {
              if(err) 
              {
                throw err;
              }
              if(results[0] == undefined)
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
      })
    });
  }
));

// Setup TLS
var express_options = {
key:  fs.readFileSync('../NovaWeb/serverkey.pem'),
	  cert: fs.readFileSync('../NovaWeb/servercert.pem'),
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


console.info("Serving static GET at /var/www");
app.use(express.static('/var/www'));

console.info("Listening on 8042");
app.listen(8042);
var nowjs = require("now");
var everyone = nowjs.initialize(app);

app.get('/configNova', ensureAuthenticated, function(req, res) {
     res.render('config.jade', 
	 {
		locals: {
			INTERFACES: config.ReadSetting("INTERFACE").split(" ")
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
	 })
});

app.get('/configHoneydNodes', ensureAuthenticated, function(req, res) {
	honeydConfig.LoadAllTemplates();
	 var nodeNames = honeydConfig.GetNodeNames();
	 var nodes = [];
	 for (var i = 0; i < nodeNames.length; i++) {
		nodes.push(honeydConfig.GetNode(nodeNames[i]));
	 }
     
	 res.render('configHoneyd.jade', 
	 { locals: {
	 	profiles: honeydConfig.GetProfileNames()
	 	,nodes: nodes
		,subnets:  honeydConfig.GetSubnetNames() 	
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

	res.render('editHoneydProfile.jade', 
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
	res.render('customizeTraining.jade',
	{ locals : {
		desc: trainingDb.GetDescriptions()
		, uids: trainingDb.GetUIDs()
		, hostiles: trainingDb.GetHostile()	
	}})
});

app.get('/configWhitelist', ensureAuthenticated, function(req, res) {
	res.render('configWhitelist.jade',
	{ locals : {
		whitelistedIps: whitelistConfig.GetIps(),
		whitelistedRanges: whitelistConfig.GetIpRanges()
	}})
});

app.get('/logout', function(req, res){
  req.logout();
  res.redirect('/');
});

app.get('/novaMain', ensureAuthenticated, function(req, res) {
     res.render('main.jade', 
     {
         user: req.user
     });
});

app.get('/login', function(req, res){
     res.render('login.jade',
     {
         user: req.user
         , message: req.flash('error')  
     });
});

app.get('/', ensureAuthenticated, function(req, res) {
     res.render('main.jade', 
     {
         user: req.user
         , message: req.flash('error')    
     });
});

app.post('/login', 
  passport.authenticate('local', { failureRedirect: '/', failureFlash: true }), 
    function(req, res){
        res.redirect('/novaMain');
});

app.post('/customizeTrainingSave', ensureAuthenticated, function(req, res){
  for(var uid in req.body)
  {
      trainingDb.SetIncluded(uid, true);
  }
	
	trainingDb.Save();
	
	res.render('saveRedirect.jade', { locals: {redirectLink: "'/customizeTraining'"}})	
});

app.post('/editHoneydNodesSave', ensureAuthenticated, function(req, res) {
	var ipAddress;
	if (req.body["ipType"] == "DHCP") {
		ipAddress = "DHCP";
	} else{
		ipAddress = req.body["ip1"] + "." + req.body["ip2"] + "." + req.body["ip3"] + "." + req.body["ip4"];
	}
	/*else
	{
	  res.redirect('/configHoneydNodes', { locals: { message: "Invalid IP" }} );
	}*/

	var profile = req.body["profile"];
	var intface = req.body["interface"];
	var subnet = "";
	var count = Number(req.body["nodeCount"]);

	console.log("Creating new nodes:" + profile + " " + ipAddress + " " + intface + " " + count);
	honeydConfig.AddNewNodes(profile, ipAddress, intface, subnet, count);
	honeydConfig.SaveAllTemplates();
     
	res.render('saveRedirect.jade', { locals: {redirectLink: "'/configHoneydNodes'"}})

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
	var configItems = ["INTERFACE","HS_HONEYD_CONFIG","TCP_TIMEOUT","TCP_CHECK_FREQ","READ_PCAP","PCAP_FILE",
		"GO_TO_LIVE","CLASSIFICATION_TIMEOUT","SILENT_ALARM_PORT","K","EPS","IS_TRAINING","CLASSIFICATION_THRESHOLD","DATAFILE",
		"SA_MAX_ATTEMPTS","SA_SLEEP_DURATION","USER_HONEYD_CONFIG","DOPPELGANGER_IP","DOPPELGANGER_INTERFACE","DM_ENABLED",
		"ENABLED_FEATURES","TRAINING_CAP_FOLDER","THINNING_DISTANCE","SAVE_FREQUENCY","DATA_TTL","CE_SAVE_FILE","SMTP_ADDR",
		"SMTP_PORT","SMTP_DOMAIN","RECIPIENTS","SERVICE_PREFERENCES","HAYSTACK_STORAGE"];
  
  var Validator = require('validator').Validator;
  
  Validator.prototype.error = function(msg)
  {
    this._errors.push(msg);
  }
  
  Validator.prototype.getErrors = function()
  {
    return this._errors;
  }
  
  var validator = new Validator();
  
  
  for(var item = 0; item < configItems.length; item++)
  {
    switch(configItems[item])
    {
      case "SA_SLEEP_DURATION":
        validator.check(req.body[configItems[item]], 'Must be a nonnegative integer or floating point number').isFloat();
        break;
        
      case "TCP_TIMEOUT":
        validator.check(req.body[configItems[item]]).isInt();
        break;
        
      case "ENABLED_FEATURES":
        validator.check(req.body[configItems[item]], 'Enabled Features mask must be nine characters long').len(9, 9);
        validator.check(req.body[configItems[item]], 'Enabled Features mask must contain only 1s and 0s').regex('[0-1]{9}');
        break;
        
      case "CLASSIFICATION_THRESHOLD":
        validator.check(req.body[configItems[item]], 'Classification threshold must be a floating point value').isFloat();
        validator.check(req.body[configItems[item]], 'Classification threshold must be a value between 0 and 1').max(1);
        break;
        
      case "EPS":
        validator.check(req.body[configItems[item]], 'EPS must be a number').isFloat();
        break;
        
      case "THINNING_DISTANCE":
        validator.check(req.body[configItems[item]], 'Thinning Distance must be a number').isFloat();
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
        //  \\.)*(([A-z]|[0-9])*)\\@((([A-z]|[0-9])*)\\.)*(([A-z]|[0-9])*)\\.(([A-z]|[0-9])*)
        validator.check(req.body[configItems[item]], 'SMTP Address is the wrong format').regex('(([A-z]|[0-9])*\\.)*(([A-z]|[0-9])*)\\@((([A-z]|[0-9])*)\\.)*(([A-z]|[0-9])*)\\.(([A-z]|[0-9])*)');
        break;
      /*
      case "SMTP_DOMAIN":
        validator.check(req.body[configItems[item]], 'SMTP Domain is the wrong format').regex('(([A-z]|[0-9])*\\.)+(([A-z]|[0-9])*)');
        break;
        
      case "RECIPIENTS":
        validator.check(req.body[configItems[item]], 'Recipients list should be a comma separated list of email addresses').regex('(((([A-z]|[0-9])*\\.)*(([A-z]|[0-9])*)\\@((([A-z]|[0-9])*)\\.)*(([A-z]|[0-9])*)\\.(([A-z]|[0-9])*))\\,)*(((([A-z]|[0-9])*\\.)*(([A-z]|[0-9])*)\\@((([A-z]|[0-9])*)\\.)*(([A-z]|[0-9])*)\\.(([A-z]|[0-9])*)))');
        break;
      */
      case "SERVICE_PREFERENCES":
        validator.check(req.body[configItems[item]], "Service preferences string is of the wrong format").is('0:[0-7](\\+|\\-)?;1:[0-7](\\+|\\-)?;2:[0-7](\\+|\\-)?;');
        break;
        
      default:
        break;
    }
  }
  
  
  var errors = validator.getErrors();
  
  if(errors.length > 0)
  {
    res.render('error.jade', { locals: {errorDetails: errors, redirectLink: "/configNova"} });
  }
  else
  {
    //if no errors, send the validated form data to the WriteSetting method
    for(var item = 0; item < configItems.length; item++)
    {
      config.WriteSetting(configItems[item], req.body[configItems[item]]);  
    }
    
    res.render('saveRedirect.jade', { locals: {redirectLink: "'/configNova'"}}) 
  }
});

// Functions to be called by clients
everyone.now.ClearAllSuspects = function(callback)
{
	console.log("Attempting to clear all suspects in main.js");

	nova.CheckConnection();
	nova.ClearAllSuspects();
}



everyone.now.StartHaystack = function(callback)
{
	nova.StartHaystack();
}

everyone.now.StopHaystack = function(callback)
{
	nova.StopHaystack();
}



everyone.now.StartNovad = function(callback)
{
	nova.StartNovad();
	nova.CheckConnection();
}

everyone.now.StopNovad = function(callback)
{
	nova.StopNovad();
	nova.CloseNovadConnection();
}


everyone.now.sendAllSuspects = function(callback)
{
	nova.getSuspectList(callback);
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
    profile.ethernet = profile.GetEthernet();
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

    callback(profile);
}

everyone.now.GetPorts = function (profileName, callback) {
    var ports = honeydConfig.GetPorts(profileName);
    for ( var i = 0; i < ports.length; i++) {
      ports[i].portName = ports[i].GetPortName();
      ports[i].portNum = ports[i].GetPortNum();
      ports[i].type = ports[i].GetType();
      ports[i].behavior = ports[i].GetBehavior();
      ports[i].scriptName = ports[i].GetScriptName();
      ports[i].isInherited = ports[i].GetIsInherited();
    }

    callback(ports);
}


everyone.now.SaveProfile = function(profile, ports, callback) {
	honeydProfile = new novaconfig.HoneydProfileBinding();

	console.log("Got profile " + profile.name);
	console.log("Got portlist " + ports.name);

	// Move the Javascript object values to the C++ object
	honeydProfile.SetName(profile.name);
	honeydProfile.SetTcpAction(profile.tcpAction);
	honeydProfile.SetUdpAction(profile.udpAction);
	honeydProfile.SetIcmpAction(profile.icmpAction);
	honeydProfile.SetPersonality(profile.personality);
	honeydProfile.SetEthernet(profile.ethernet);
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

	honeydConfig.SaveAllTemplates();

	// Save the profile
	honeydProfile.Save();

	callback();
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
  if (req.isAuthenticated()) { return next(); }
  else { res.redirect('/login'); }
}

function queryCredDb(check) {
    console.log("checkPass value before queryCredDb call: " + check);
    
    client.query(
    'SELECT pass FROM ' + credTb + ' WHERE pass = PASSWORD(\'' + check + '\')',
    function selectCb(err, results, fields) {
      if(err) {
        throw err;
      }
      
      select = results[0].pass;
      
      console.log("queryCredDb results: " + select);
      
      if(select === results[0].pass)
      {
        console.log("all good");
        return true;
      }
      else
      {
        console.log("Username password combo incorrect");
        return false;
      }
    }
  );
};

/*function getPassHash(password, fn) {
      client.query(
      'SELECT PASSWORD(\'' + password + '\') AS pass',
      function selectCb(err, results, fields) {
        if(err) {
          throw err;
        }
        
        checkPass = results[0].pass;
        
        console.log("getPassHash results: " + checkPass);
        
        if(checkPass != undefined)
        {
          console.log("getPassHash success");
          return fn(password);
        }
        else
        {
          console.log("getPassHash failed");
          client.end();
          return false;
        }
      }
    );
};*/

function checkUsername(userName, fn) {
  client.query(
    'SELECT user FROM ' + credTb + ' WHERE user = \'' + userName + '\'',
    function selectCb(err, results, fields) {
      if(err) {
        throw err;
      }
      if(results[0] == undefined)
      {
        return fn(null, null);
      } 
      else if(userName != results[0].user)
      {
        return fn(null, null);
      }
      else
      {
        return fn(null, userName);
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

