
var nova = require('nova.node');
var n = new nova.Instance();
console.info("Nova up: " + n.IsUp());
if( ! n.IsUp() )
{
    console.info("Start Novad: " + n.StartNovad());
}

n.OnNewSuspect(function(suspect)
{ 
    console.info("New suspect: " + suspect.GetInAddr());
    console.info("Suspect list: " + n.getSuspectList());
});

var fs = require('fs');
var express=require('express');
var https = require('https');

// Setup TLS
var express_options = {
key:  fs.readFileSync('serverkey.pem'),
cert: fs.readFileSync('servercert.pem'),
};

var app = express.createServer(express_options);

console.info("Logging to ./serverLog.log");
var logFile = fs.createWriteStream('./serverLog.log', {flags: 'a'});
app.use(express.logger({stream: logFile}));

console.info("Serving static GET at /var/www");
app.use(express.static('/var/www'));

console.info("Listening on 8080");
app.listen(8080);

var everyone = require("now").initialize(app);
everyone.now.IsUp = function(callback) { 
    callback(n.IsUp());
}



process.on('SIGINT', function()
{
    n.Shutdown();
    process.exit();
});