var novaconfig = require('novaconfig.node');

// TODO: Need to make fallback code, in case a connection
// doesn't/can't accept websocket connections (for whatever reason)
// Problem not a problem now, but definitely will be later
var express = require('express');
var app = require('express').createServer();
var nowjs = require('now');
var fs = require('fs');
var nova = new novaconfig.Instance();
var config = new novaconfig.NovaConfigBinding();

var NovaHomePath = config.GetPathHome();

// Configure the express server to use the 
// bodyParser so that we can view and use the 
// contents of the req variable in the get/post
// directives
app.configure(function()
{
	app.use(express.bodyParser());
	app.use(app.router);
});

// Initailize the object that will store the connections from the various
// Quasar clients
var novaClients = new Object();
var fileAssociations = new Array();

// Set the various view options, as well as the listening port
// and the directory for the server and the jade files to look for 
// src files, etc.
app.set('view options', { layout: false });
app.set('views', __dirname + '/views');
// TODO: Make port configurable
app.listen(8080);
app.use(express.static(NovaHomePath + '/../Quasar/www'));

// Initialize nowjs to listen to our express server
var everyone = nowjs.initialize(app);

// Generic message sending method; can be called from the jade files
// as well as on the server side. Parsing for messages sent from here
// is handled on the Quasar side.
everyone.now.MessageSend = function(message) 
{
    var targets = message.id.split(':');
    for(var i in targets)
    {
        if(targets[i] != '')
        {
	        novaClients[targets[i]].sendUTF(JSON.stringify(message));
        }
    }
};

everyone.now.RemoveGroup = function(group)
{
  console.log('Removing group ' + group + ' from the client groups file');
  var groupFile = fs.readFileSync(NovaHomePath + '/../Mothership/client_groups.txt', 'utf8');
  var regex = group + ".+?;";
  var replaceWithNull = new RegExp(regex, "g");
  groupFile = groupFile.replace(replaceWithNull, '');
  groupFile = trimNewlines(groupFile);
  fs.writeFileSync(NovaHomePath + '/../Mothership/client_groups.txt', groupFile);
};

everyone.now.UpdateGroup = function(group, newMembers)
{
  console.log('Updating group ' + group + ' to have members ' + newMembers);
  var groupFile = fs.readFileSync(NovaHomePath + '/../Mothership/client_groups.txt', 'utf8');
  var regex = group + ".+?;";
  var replaceWithNull = new RegExp(regex, "g");
  groupFile = groupFile.replace(replaceWithNull, group + ":" + newMembers + ";");
  fs.writeFileSync(NovaHomePath + '/../Mothership/client_groups.txt', groupFile);
}

everyone.now.AddGroup = function(group, members)
{
  console.log('Adding group ' + group + ' with members ' + newMembers);
  var groupFile = fs.readFileSync(NovaHomePath + '/../Mothership/client_groups.txt', 'utf8');
  groupFile += '\n' + group + ":" + newMembers + ";";
  fs.writeFileSync(NovaHomePath + '/../Mothership/client_groups.txt', groupFile);
}

function trimNewlines(string)
{
  var ret = string;
  if(string[0] == '\n')
  {
    ret = string.substr(1);
  }
  return ret;
}

// Begin setup of websocket server. Going to have an 
// https base server so that we can use wss:// connections
// instead of basic ws://
var WebSocketServer = require('websocket').server;
var https = require('https');

// Eventually we will want these options to include the
// commented stanzas; the ca option will contain a list of 
// trusted certs, and the other options do what they say
// TODO: These paths will need to be relative to the NovaPath
// or something mothership specific
var options = {
	key: fs.readFileSync('/usr/share/nova/Quasar/serverkey.pem'),
	cert: fs.readFileSync('/usr/share/nova/Quasar/servercert.pem')
	/*ca: fs.readFileSync(''),
	requestCert:		true,
	rejectUnauthorized:	false*/
};

// Create the httpsServer, and make it so that any requests for urls over http
// to the server are met with 404s; after the initial handshake, we shouldn't be 
// dealing directly with http, we should let websockets do the work. We may want to 
// look into https as a fallback, however, in case websockets is unsupported/firewalled
// etc.
var httpsServer = https.createServer(options, function(request, response)
{
	console.log('Recieved request for url ' + request.url);
	response.writeHead(404);
	response.end();
});

// Have to have the websockets listen on a different port than the express
// server, or else it'll catch all the messages that express is meant to get
// and lead to some undesirable behavior
// TODO: Make this port configurable
httpsServer.listen(8081, function()
{
	console.log('Mothership Server is listening on 8081');
});

// Initialize the WebSocketServer to use the httpsServer as the 
// base server
var wsServer = new WebSocketServer({
	httpServer: httpsServer
});

// On request, accept the connection. I'm a little wary of the way this is 
// structured (in terms of connection acceptance) but I've made Quasar attempt to 
// connect over ws://, which didn't work (a good thing) as well as trying to connect
// with wss:// but bad certs, which didn't work either (double good). I think there's 
// a lot of back-end warlockery that happens to moderate authentication, but we may want
// to find an explicit way to manage it purely for code readability's sake.
wsServer.on('request', function(request)
{
	console.log('connection accepted');
	var connection = request.accept(null, request.origin);
    // The most important directive, if we have a message, we need to parse it out
    // and determine what to do from there
	connection.on('message', function(message){
        // Probably not needed, but here for kicks
		if(message.type === 'utf8')
		{
            // Parse the message data into a JSON object
			var json_args = JSON.parse(message.utf8Data);
            // If the parsing went well...
			if(json_args != undefined)
			{
                // ...then look at the message type and switch from there
                // A note on the message types. The string that is put into the 
                // json_args.type JSON member on the client side MUST MATCH CASE 
                // EXACTLY with the case strings
				switch(json_args.type)
				{
                    // addId is the first message a client sends to the mothership,
                    // essentially binds their client id to the connection that has
                    // has been created for future reference and connection management
					case 'addId':
						// TODO: Check that id doesn't exist before adding
						novaClients[json_args.id.toString()] = connection;
						console.log('Connected clients: ');
						for(var i in novaClients)
						{
							console.log('client ' + i);
						}
                        // TODO: Get hostile suspects from connected client
                        var getHostile = {};
                        getHostile.type = 'getHostileSuspects';
                        getHostile.id = json_args.id + ':';
                        everyone.now.MessageSend(getHostile);
						break;
                    // This case is reserved for response from the clients;
                    // we should figure out a standard format for the responses 
                    // that includes the client id and the message, at the very least
					case 'response':
						console.log(json_args.response_message);
						break;
                    // This case is reserved for receiving and properly addressing
                    // hostile suspect events from a client. Parses the message out into
                    // an object literal which is then sent to the OnNewSuspect event. 
                    // Might be able to get away with just sending the JSON object, my intent
                    // was to validate the message here with conditionals, just isn't done yet.
					case 'hostileSuspect':
                        console.log('suspect received');
						var suspect = {};
						suspect.string = json_args.string;
						suspect.ip = json_args.ip;
						suspect.classification = json_args.classification;		
						suspect.lastpacket = json_args.lastpacket;
						suspect.ishostile = json_args.ishostile;
						suspect.client = json_args.client;
						everyone.now.OnNewSuspect(suspect);
						break;
					case 'registerConfig':
						console.log('Nova Configuration received from ' + json_args.id);
						// TODO: Check for existing client to add new file association to, extend the .file 
						//       part of the push object literal
						var push = {};
						push.client = json_args.id;
						push.file = (NovaHomePath + '/../Mothership/ClientConfigs/' + json_args.filename);
						fileAssociations.push(push);
						fs.writeFileSync(push.file, json_args.file);
						console.log('Configuration for ' + json_args.id + ' can be found at ' + json_args.filename);
						break;
					// If we've found a message type that we weren't expecting, or don't have a case
                    // for, log this message to the console and do nothing.
					default:
						console.log('Unexpected/Unknown message type ' + json_args.type + ' received, doing nothing');
						break;
				}
			}
            // TODO: Account for non-UTF8 here
            else
            {

            }
		}	
	});
});


// If the connection to the server is dropped for some reason, just print a description.
// The client side will handle reconnects. Still need to find a way to remove connections 
// from novaClients when this action is hit; since we don't have the client id to use,
// however, it may be tough. Have to look and see if the connection passed in can be matched
// to elements in the object and bound that way
wsServer.on('close', function(connection, reason, description)
{
	console.log('Closed connection: ' + description);
    for(var i in novaClients)
    {
        if(novaClients[i] === connection)
        {
            delete novaClients[i];
        }
    }
});

// A function to get a string representation of the clients list 
// from the novaClients object. Probably could do it with just 
// an array, but since the intent is to pass it to jade which will
// just make a string out of it anyways, I figured I'd cut out the 
// middle man.
function getClients()
{
	var ret = '';
	for(var i in novaClients)
	{
		if(i != undefined)
		{
			ret += (i + ':');
		}	
	}
	return ret;
}


// A function that reads the file client_groups.txt that contains user-created groups names
// that have an associated list of last-known clientIds. 
function getGroups()
{
  var group = fs.readFileSync(NovaHomePath + '/../Mothership/client_groups.txt', 'ascii');
  if(group == '')
  {
    console.log('client groups file is empty');
  }
  else
  {
    var clientGroups = group.split(';');
    var ret = {};
    ret.groups = '';
    ret.members = '';
    
    for(var i = 0; i < clientGroups.length; i++)
    {
      if(clientGroups[i] != '' && clientGroups[i].length > 1)
      {
        var members = clientGroups[i].split(':');
        ret.groups += members[0].replace(/(\r\n|\n|\r)/gm,"") + ':';
        ret.members += members[1] + '|';
      }
    }
  }
  
  return JSON.stringify(ret);
}

// Going to need to do passport for these soon, I think.
app.get('/', function(req, res) 
{
	res.render('main.jade', {locals:{
		clients: getClients()
	}});
});

app.get('/about', function(req, res) 
{
    res.render('about.jade');
});

app.get('/config', function(req, res) 
{
	res.render('config.jade', {locals:{
		CLIENTS: getClients()
		, TCP_TIMEOUT: config.ReadSetting('TCP_TIMEOUT')
		, TCP_CHECK_FREQ: config.ReadSetting('TCP_CHECK_FREQ')
		, READ_PCAP: config.ReadSetting('READ_PCAP')
		, GO_TO_LIVE: config.ReadSetting('GO_TO_LIVE')
		, CLASSIFICATION_TIMEOUT: config.ReadSetting('CLASSIFICATION_TIMEOUT')
		, K: config.ReadSetting('TK')
		, EPS: config.ReadSetting('EPS')
		, CLASSIFICATION_THRESHOLD: config.ReadSetting('CLASSIFICATION_THRESHOLD')
		, DM_ENABLED: config.ReadSetting('DM_ENABLED')
		, ENABLED_FEATURES: config.ReadSetting('ENABLED_FEATURES')
		, FEATURE_NAMES: nova.GetFeatureNames()
		, THINNING_DISTANCE: config.ReadSetting('THINNING_DISTANCE')
		, SAVE_FREQUENCY: config.ReadSetting('SAVE_FREQUENCY')
		, DATA_TTL: config.ReadSetting('DATA_TTL')
		, SERVICE_PREFERENCES: config.ReadSetting('SERVICE_PREFERENCES')
		, CAPTURE_BUFFER_SIZE: config.ReadSetting('CAPTURE_BUFFER_SIZE')
		, MIN_PACKET_THRESHOLD: config.ReadSetting('MIN_PACKET_THRESHOLD')
		, CUSTOM_PCAP_FILTER: config.ReadSetting('CUSTOM_PCAP_FILTER')
		, CUSTOM_PCAP_MODE: config.ReadSetting('CUSTOM_PCAP_MODE')
		, CLEAR_AFTER_HOSTILE_EVENT: config.ReadSetting('CLEAR_AFTER_HOSTILE_EVENT')
	}});
});

app.get('/groups', function(req, res){
  res.render('groups.jade', {locals:{
    CLIENTS: getClients()
    , GROUPS: getGroups()
  }});
});
