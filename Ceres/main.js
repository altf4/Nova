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
var ws = require('websocket-server');
var http = require('http');

var hostConnection = {};

var wsServer = ws.createServer();

wsServer.addListener('connection', function(client){
  console.log('connection attempt');
  client.addListener('message', function(message){
    var parsed = JSON.parse(message);
    console.log('parsed.type == ' + parsed.type);
    switch(parsed.type)
    {
      case 'getAll':
        client.send('<suspect ipaddress="192.168.11.10" interface="eth0">50</suspect>');
        break;
      default: 
        console.log('switched with no problem');
        break;
    }
  });
});

wsServer.listen(8080);

