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
        setTimeout(function(){
          NovaCommon.nova.sendSuspectListArray(function(suspects){
            gridPageSuspectList(suspects, function(xml){
              client.send(xml);
            });
          });
        }, 2000);
        break;
      default: 
        console.log('switched with no problem');
        break;
    }
  });
});

wsServer.listen(8080);

function gridPageSuspectList(suspects, cb){
  var suspectRet = '<suspects>';
  var js2xmlopt = {'declaration':{'include':false}, 'prettyPrinting':{'enabled':false}};
  for(var i in suspects)
  {
    var ip = suspects[i].GetIpString();
    var iface = suspects[i].GetInterface();
    var classification = (Math.floor(parseFloat(suspects[i].GetClassification()) * 10000) / 100).toFixed(2);
    if(classification == '-200.00')
    {
      continue;
    }
    var suspectXmlTemplate = {'@':{'ipaddress':ip, 'interface':iface}, '#':classification};
    suspectRet += j2xp('suspect', suspectXmlTemplate, js2xmlopt) + '>';
  }
  suspectRet += '</suspects>'
  cb && cb(suspectRet);
}

