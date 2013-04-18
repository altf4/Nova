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
// Description : Main node.js file for the Ceres web interface of Nova
//============================================================================

var NovaCommon = require('./NovaCommon.js');
var BasicStrategy = require('passport-http').BasicStrategy;
var WebSocketServer = require('websocket').server;
var http = require('http');

var hostConnection = {};

var httpServer = http.createServer(function(request, response){
  console.log('Recieved request for url ' + request.url);
  response.writeHead(404);
  response.end();
});

httpServer.listen(8080, function(){
  console.log('Ceres Server is listening on ' + 8080);
});

var wsServer = new WebSocketServer({
  httpServer: httpServer
});

wsServer.on('request', function(request){
  var connection = request.accept(null, request.origin);
  hostConnection[connection] = request.remoteAddress;
  connection.on('message', function(message){
    var messageStructure = JSON.parse(message.data);
    console.log('messageStructure == ' + JSON.stringify(messageStructure));
    switch(messageStructure)
    {
      
    }
  });
});

wsServer.on('close', function(connection, reason, description){
  console.log('Closed connection: ' + description);
  delete hostConnection[connection];
});
