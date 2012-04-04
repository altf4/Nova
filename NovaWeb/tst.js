
var novaNode = require('nova.node');
var nova = new novaNode.Instance();
console.info("Nova up: " + nova.IsNovadUp(true));
//if( ! nova.IsNovadUp() )
//{
//    console.info("Start Novad: " + nova.StartNovad());
//}

nova.OnNewSuspect(function(suspect)
{ 
    console.info("New suspect: " + suspect.GetInAddr());
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

// Do the following for logging
//console.info("Logging to ./serverLog.log");
//var logFile = fs.createWriteStream('./serverLog.log', {flags: 'a'});
//app.use(express.logger({stream: logFile}));

console.info("Serving static GET at /var/www");
app.use(express.static('/var/www'));

console.info("Listening on 8042");
app.listen(8042);

var everyone = require("now").initialize(app);

// Functions to be called by clients
everyone.now.IsNovadUp = function(callback) 
{ 
    callback(nova.IsNovadUp(false));
}

everyone.now.IsHaystackUp = function(callback) 
{ 
    callback(nova.IsHaystackUp());
}

everyone.now.ClearAllSuspects = function(callback)
{
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
}

everyone.now.StopNovad = function(callback)
{
    nova.StopNovad();
}


everyone.now.OnNewSuspect = function(callback) 
{   
    nova.OnNewSuspect(function(suspect){ 
            console.log("Calling back client with suspect: " + suspect.GetInAddr());            
            var s = new Object();
            objCopy(suspect,s);
            callback(s); 
        });
}

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
