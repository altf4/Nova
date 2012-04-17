var novaNode = require('./build/Release/nova.node');
var nova = new novaNode.Instance();

var novaconfig = require('./NodeNovaConfig/build/Release/novaconfig');
var config = new novaconfig.NovaConfigBinding();

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

console.info("Listening on 8042");
app.listen(8042);
var everyone = require("now").initialize(app);
		
		
// Nova config stuff		
var configItems = [
	"INTERFACE",
	"HS_HONEYD_CONFIG",
	"TCP_TIMEOUT",
	"TCP_CHECK_FREQ",
	"READ_PCAP",
	"PCAP_FILE",
	"GO_TO_LIVE",
	"CLASSIFICATION_TIMEOUT",
	"SILENT_ALARM_PORT",
	"K",
	"EPS",
	"IS_TRAINING",
	"CLASSIFICATION_THRESHOLD",
	"DATAFILE",
	"SA_MAX_ATTEMPTS",
	"SA_SLEEP_DURATION",
	"USER_HONEYD_CONFIG",
	"DOPPELGANGER_IP",
	"DOPPELGANGER_INTERFACE",
	"DM_ENABLED",
	"ENABLED_FEATURES",
	"TRAINING_CAP_FOLDER",
	"THINNING_DISTANCE",
	"SAVE_FREQUENCY",
	"DATA_TTL",
	"CE_SAVE_FILE",
	"SMTP_ADDR",
	"SMTP_PORT",
	"SMTP_DOMAIN",
	"RECIPIENTS",
	"SERVICE_PREFERENCES",
	"HAYSTACK_STORAGE"
];


app.post('/configureNovaSave', function(req, res) {
	var result = true;
	for (var item = 0; item < configItems.length; item++) {
		if (req.body[configItems[item]] !== undefined) {
			if (!config.WriteSetting(configItems[item], req.body[configItems[item]])) {
				result = false;
			}
		}
	}
	
	res.writeHead(200, "OK", {'Content-Type': 'text/html'});
	if (result) {
		res.write("Settings saved! You will be redirected back to the settings page...");
	} else {
		res.write("There was an error when attempting to write settings to configuration file");
	}
	res.end();
});

app.get('/configureNova', function(req, res) {
		console.log("[200] " + req.method + " to " + req.url);
		res.writeHead(200, "OK", {'Content-Type': 'text/html'});
		res.write('<HTML>');
		res.write('<HEAD>');
		res.write(' <link rel="stylesheet" type="text/css" href="configstyle.css" media="screen">');
		res.write('</HEAD>');
		res.write('<BODY>');

		res.write('<H1> NOVA Configuration </H1>');

		for (var item = 0; item < configItems.length; item++) {
			res.write('<label>');
			res.write(configItems[item]);
			res.write('</label>');
			res.write('<form method="post" action="/configureNovaSave">');
			res.write('<input type="text" name="');
			res.write(configItems[item]);
			res.write('" value="');
			res.write(config.ReadSetting(configItems[item]));
			res.write('" /><br />');
		}

		res.write('<input type="submit" name="submitbutton" id="submitbutton" value="Save Settings" />');
		res.write('</form>');
		res.write("<br/>");
		res.write('</BODY>');
		res.write('</HTML>');

		res.end();
});



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
		everyone.now.updateHaystackStatus(nova.IsHaystackUp());
		everyone.now.updateNovadStatus(nova.IsNovadUp(false));
}, 5000);



