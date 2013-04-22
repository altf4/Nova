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
var j2xp = require('js2xmlparser');
var http = require('http');

var hostConnection = {};

NovaCommon.nova.CheckConnection();
NovaCommon.StartNovad(false);

var wsServer = ws.createServer();

wsServer.addListener('connection', function(client){
  client.addListener('message', function(message){
    var parsed = JSON.parse(message);
    switch(parsed.type)
    {
      case 'getAll':
        hostConnection[client] = parsed.id;
        NovaCommon.nova.CheckConnection();
        var suspects = {};
        NovaCommon.nova.sendSuspectList(function(suspect){
          console.log('************************* in sendSuspectList *************************');
          var ip = suspect.GetIpString;
          var iface = suspect.GetInterface;
          var classification = suspect.GetClassification;
          var suspectXmlTemplate = {'suspect':[{'@':{'ipaddress':ip, 'interface':iface}, '#':classification}]};
          var js2xmlopt = {'prettyPrinting':false};
          console.log('xxxYOYOYOxxx j2xp suspect == ' + j2xp('suspects', suspectXmlTemplate, js2xmlopt));
        });
        //client.send('<suspects><suspect ipaddress="192.168.11.10" interface="eth0">50</suspect><suspect ipaddress="255.255.255.255" interface="eth0">50</suspect></suspects>');
        break;
      default: 
        console.log('switched with no problem');
        break;
    }
  });
});

wsServer.listen(8080);

