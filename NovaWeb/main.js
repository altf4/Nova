var novaconfig = require('novaconfig.node');




var nova = new novaconfig.Instance();
var config = new novaconfig.NovaConfigBinding();
var honeydConfig = new novaconfig.HoneydConfigBinding();
var vendorToMacDb = new novaconfig.VendorMacDbBinding();
var osPersonalityDb = new novaconfig.OsPersonalityDbBinding();
var trainingDb = new novaconfig.CustomizeTrainingBinding();

honeydConfig.LoadAllTemplates();

var fs = require('fs');
var jade = require('jade');
var express=require('express');
var https = require('https');

// Setup TLS
var express_options = {
key:  fs.readFileSync('serverkey.pem'),
	  cert: fs.readFileSync('servercert.pem'),
};

var app = express.createServer(express_options);


app.configure(function () {
		app.use(express.methodOverride());
		app.use(express.bodyParser());
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


app.get('/', function(req, res) {
     res.render('main.jade');
 });

app.get('/configNova', function(req, res) {
     res.render('config.jade', 
	 {
		locals: {
			INTERFACE: config.ReadSetting("INTERFACE")
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

app.get('/configHoneydNodes', function(req, res) {
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


app.get('/configHoneydProfiles', function(req, res) {
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

app.get('/editHoneydNode', function(req, res) {
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

app.get('/editHoneydProfile', function(req, res) {
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


app.get('/addHoneydProfile', function(req, res) {
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

app.get('/customizeTraining', function(req, res) {
	res.render('customizeTraining.jade',
	{ locals : {
		desc: trainingDb.GetDescriptions()
		, uids: trainingDb.GetUIDs()
		, hostiles: trainingDb.GetHostile()	
	}})
});

app.post('/customizeTrainingSave', function(req, res){
  for(var uid in req.body)
  {
      trainingDb.SetIncluded(uid, true);
  }
	
	trainingDb.Save();
	
	res.render('saveRedirect.jade', { locals: {redirectLink: "'/customizeTraining'"}})	
});

app.post('/editHoneydNodesSave', function(req, res) {
	var ipAddress;
	if (req.body["ipType"] == "DHCP") {
		ipAddress = "DHCP";
	} else {
		ipAddress = req.body["ip1"] + "." + req.body["ip2"] + "." + req.body["ip3"] + "." + req.body["ip4"];
	}

	var profile = req.body["profile"];
	var intface = req.body["interface"];
	var subnet = "";
	var count = Number(req.body["nodeCount"]);

	console.log("Creating new nodes:" + profile + " " + ipAddress + " " + intface + " " + count);
	honeydConfig.AddNewNodes(profile, ipAddress, intface, subnet, count);
	honeydConfig.SaveAllTemplates();
     
	res.render('saveRedirect.jade', { locals: {redirectLink: "'/configHoneydNodes'"}})

});

app.post('/editHoneydNodeSave', function(req, res) {
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
	honeydConfig.AddNewNode(profile, ipAddress, macAddress, intface, subnet);
	honeydConfig.SaveAllTemplates();
     
	res.render('saveRedirect.jade', { locals: {redirectLink: "'/configHoneydNodes'"}})

});

app.post('/configureNovaSave', function(req, res) {
	// TODO: Throw this out and do error checking in the Config (WriteSetting) class instead
	var configItems = ["INTERFACE","HS_HONEYD_CONFIG","TCP_TIMEOUT","TCP_CHECK_FREQ","READ_PCAP","PCAP_FILE",
		"GO_TO_LIVE","CLASSIFICATION_TIMEOUT","SILENT_ALARM_PORT","K","EPS","IS_TRAINING","CLASSIFICATION_THRESHOLD","DATAFILE",
		"SA_MAX_ATTEMPTS","SA_SLEEP_DURATION","USER_HONEYD_CONFIG","DOPPELGANGER_IP","DOPPELGANGER_INTERFACE","DM_ENABLED",
		"ENABLED_FEATURES","TRAINING_CAP_FOLDER","THINNING_DISTANCE","SAVE_FREQUENCY","DATA_TTL","CE_SAVE_FILE","SMTP_ADDR",
		"SMTP_PORT","SMTP_DOMAIN","RECIPIENTS","SERVICE_PREFERENCES","HAYSTACK_STORAGE"];

	var result = true;
	for (var item = 0; item < configItems.length; item++) {
		if (req.body[configItems[item]] !== undefined) {
			if (!config.WriteSetting(configItems[item], req.body[configItems[item]])) {
				result = false;
			}
		}
	}
	
	res.render('saveRedirect.jade', { locals: {redirectLink: "'/configNova'"}})	
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


setInterval(function() {
		try {
			everyone.now.updateHaystackStatus(nova.IsHaystackUp());
			everyone.now.updateNovadStatus(nova.IsNovadUp(false));
		} catch (err)
		{

		}
}, 5000);

