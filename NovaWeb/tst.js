var novaNode = require('./build/Release/nova.node');
var nova = new novaNode.Instance();
console.log("STARTUP: Created a new instance of novaNode");

var novaconfig = require('./NodeNovaConfig/build/Release/novaconfig');
var config = new novaconfig.NovaConfigBinding();

//console.info("Nova up: " + nova.IsNovadUp(true));
//if( ! nova.IsNovadUp() )
//{
//    console.info("Start Novad: " + nova.StartNovad());
//}

//nova.OnNewSuspect(function(suspect)
//{ 
//    console.info("New suspect: " + suspect.GetInAddr());
//});

var fs = require('fs');
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


// Do the following for logging
//console.info("Logging to ./serverLog.log");
//var logFile = fs.createWriteStream('./serverLog.log', {flags: 'a'});
//app.use(express.logger({stream: logFile}));


console.info("Serving static GET at /var/www");
app.use(express.static('/var/www'));

app.post('/', function(req, res) {
		console.info("GOT A POST REQUEST FOR CONFIGURATION! YAY!" + "interface was " + req.body.interface);	
		});

app.get('/configureNova', function(req, res) {
	console.log("[200] " + req.method + " to " + req.url);
	res.writeHead(200, "OK", {'Content-Type': 'text/html'});
	res.write('<HTML>');
	res.write('<HEAD>');
	res.write(' <link rel="stylesheet" type="text/css" href="style.css" media="screen">');
	res.write('</HEAD>');
	res.write('<BODY>');

	res.write('<H1> NOVA Configuration </H1>');
	res.write('<form method="post" action="/">');
	res.write('<label>Interface</label>');
	res.write('<input type="text" name="interface" value="');
	res.write(config.GetInterface());
	res.write('" /><br />');

	res.write('<label>Data File</label>');
	res.write('<input type="text" name="datafile" value="');
	res.write(config.GetPathTrainingFile().toString());
	res.write('" /><br />');

	res.write('<label>Silent Alarm Port</label>');
	res.write('<input type="text" name="saport" value="');
	res.write(config.GetSaPort().toString());
	res.write('" /><br />');

	res.write('<label>Silent Alarm Connection Attempts</label>');
	res.write('<input type="text" name="saconnectattempts" value="');
	res.write(config.GetSaMaxAttempts().toString());
	res.write('" /><br />');

	res.write('<label>K</label>');
	res.write('<input type="text" name="k" value="');
	res.write(config.GetK().toString());
	res.write('" /><br />');

	res.write('<label>EPS</label>');
	res.write('<input type="text" name="EPS" value="');
	res.write(config.GetEps().toString());
	res.write('" /><br />');

	res.write('<label>Classification Timeout</label>');
	res.write('<input type="text" name="classification_timeout" value="');
	res.write(config.GetClassificationTimeout().toString());
	res.write('" /><br />');

	res.write('<label>Training Capture Folder</label>');
	res.write('<input type="text" name="training_cap_folder" value="');
	res.write(config.GetPathTrainingCapFolder());
	res.write('" /><br />');

	res.write('<label>Thinning Distance</label>');
	res.write('<input type="text" name="thinning_distance" value="');
	res.write(config.GetThinningDistance().toString());
	res.write('" /><br />');

	res.write('<label>Classification Threshold</label>');
	res.write('<input type="text" name="classification_threshold" value="');
	res.write(config.GetClassificationThreshold().toString());
	res.write('" /><br />');

	res.write('<label>User Honeyd Config</label>');
	res.write('<input type="text" name="user_honeyd_config" value="');
	res.write(config.GetPathConfigHoneydUser().toString());
	res.write('" /><br />');

	res.write('<label>Doppelganger IP Address</label>');
	res.write('<input type="text" name="Doppelganger_ip" value="');
	res.write(config.GetDoppelIp().toString());
	res.write('" /><br />');

	res.write('<label>Doppelganger Interface</label>');
	res.write('<input type="text" name="doppelganger_interface" value="');
	res.write(config.GetDoppelInterface().toString());
	res.write('" /><br />');

	res.write('<label>Haystack Honeyd Config</label>');
	res.write('<input type="text" name="hs_honeyd_config" value="');
	res.write(config.GetPathConfigHoneydHS().toString());
	res.write('" /><br />');

	res.write('<label>TCP Timeout</label>');
	res.write('<input type="text" name="tcp_timeout" value="');
	res.write(config.GetPathTrainingFile().toString());
	res.write('" /><br />');

	res.write('<label>TCP Check Frequency</label>');
	res.write('<input type="text" name="tcp_check_freq" value="');
	res.write(config.GetTcpCheckFreq().toString());
	res.write('" /><br />');

	res.write('<label>Pcap file path</label>');
	res.write('<input type="text" name="pcac_file" value="');
	res.write(config.GetPathPcapFile().toString());
	res.write('" /><br />');

	res.write('<label>Silent Alarm Max connection attempts</label>');
	res.write('<input type="text" name="sa_max_attempts" value="');
	res.write(config.GetSaMaxAttempts().toString());
	res.write('" /><br />');

	res.write('<label>Silent Alarm Sleep Duration</label>');
	res.write('<input type="text" name="sa_sleep_duration" value="');
	res.write(config.GetSaSleepDuration().toString());
	res.write('" /><br />');

	res.write('<label>Enabled Featureset Mask</label>');
	res.write('<input type="text" name="enabled_features" value="');
	res.write(config.GetEnabledFeatures().toString());
	res.write('" /><br />');

	res.write('<label>Data TTL</label>');
	res.write('<input type="text" name="data_ttl" value="');
	res.write(config.GetDataTTL().toString());
	res.write('" /><br />');

	res.write('<label>State Save File</label>');
	res.write('<input type="text" name="ce_save_file" value="');
	res.write(config.GetPathCESaveFile().toString());
	res.write('" /><br />');

	res.write('<label>SMTP Address</label>');
	res.write('<input type="text" name="smtp_addr" value="');
	res.write(config.GetSMTPAddr().toString());
	res.write('" /><br />');

	res.write('<label>SMTP Domain</label>');
	res.write('<input type="text" name="smtp_domain" value="');
	res.write(config.GetSMTPDomain().toString());
	res.write('" /><br />');

	res.write('<label>Log Preferences</label>');
	res.write('<input type="text" name="service_preferences" value="');
	res.write(config.GetLoggerPreferences().toString());
	res.write('" /><br />');

	//res.write('<label>Haystack Storage</label>');
	//res.write('<input type="text" name="haystack_storage" value="');
	//res.write('" /><br />');

	res.write('<label>Doppelganger Enabled?</label>');
	if (config.GetIsDmEnabled()) {
		res.write('<input type="checkbox" name="dm_enabled" value="" checked/><br />');
	} else {
		res.write('<input type="checkbox" name="dm_enabled" value="" /><br />');
	}

	res.write('<label>Training?</label>');
	if (config.GetIsTraining()) {
		res.write('<input type="checkbox" name="is_training" value="" checked/><br />');
	} else {
		res.write('<input type="checkbox" name="is_training" value="" /><br />');
	}

	res.write('<label>Read Pcap?</label>');
	if (config.GetReadPcap()) {
		res.write('<input type="checkbox" name="read_pcap" value="" checked/><br />');
	} else {
		res.write('<input type="checkbox" name="read_pcap" value="" /><br />');
	}

	res.write('<label>Go to live capture?</label>');
	if (config.GetGotoLive()) {
		res.write('<input type="checkbox" name="read_pcap" value="" checked/><br />');
	} else {
		res.write('<input type="checkbox" name="read_pcap" value="" /><br />');
	}

	res.write('<input type="submit" name="submitbutton" id="submitbutton" value="Save Settings" />');
	res.write('</form>');
	res.write('</BODY>');
	res.write('</HTML>');

	res.end();
});



console.info("Listening on 8042");
app.listen(8042);



var everyone = require("now").initialize(app);




// Functions to be called by clients
everyone.now.ClearAllSuspects = function(callback)
{
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


var distributeSuspect = function(suspect)
{
	//console.log("Sending suspect to clients: " + suspect.GetInAddr());            
	var s = new Object();
	objCopy(suspect, s);
	everyone.now.OnNewSuspect(s)
};
nova.registerOnNewSuspect(distributeSuspect);
//console.log("Registered NewSuspect callback function.");


process.on('SIGINT', function()
		{
		nova.Shutdown();
		process.exit();
		});

function objCopy(src,dst)
{
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
		everyone.now.updateHaystackStatus(nova.IsHaystackUp());
		everyone.now.updateNovadStatus(nova.IsNovadUp(false));
		}, 5000);



