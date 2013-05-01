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

process.on('SIGINT', function(){
  NovaCommon.nova.Shutdown();
  process.exit();
});

process.on('exit', function(){
  console.log('Ceres has closed cleanly');
});

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
      case 'getSuspect':
        var ipiface = parsed.suspect.split(':');
        NovaCommon.nova.CheckConnection();
        setTimeout(function(){
          NovaCommon.nova.sendSuspectToCallback(ipiface[0], ipiface[1], function(suspect){
            detailedSuspectPage(suspect, function(xml){
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
    var hostile = (suspects[i].GetIsHostile() == true ? '1' : '0');
    console.log('hostile == ' + hostile);
    if(classification == '-200.00')
    {
      continue;
    }
    var suspectXmlTemplate = {'@':{'ipaddress':ip, 'interface':iface, 'hostile':hostile}, '#':classification};
    suspectRet += j2xp('suspect', suspectXmlTemplate, js2xmlopt) + '>';
  }
  suspectRet += '</suspects>'
  cb && cb(suspectRet);
}

function detailedSuspectPage(suspect, cb)
{
  var featureSet = ["IP Traffic Distribution",
                  "Port Traffic Distribution",
                  "Packet Size Mean",
                  "Packet Size Deviation",
                  "Protected IPs Contacted",
                  "Distinct TCP Ports Contacted",
                  "Distinct UDP Ports Contacted",
                  "Average TCP Ports Per Host",
                  "Average UDP Ports Per Host",
                  "TCP Percent SYN",
                  "TCP Percent FIN",
                  "TCP Percent RST",
                  "TCP Percent SYN ACK",
                  "Haystack Percent Contacted"];
  var ret = '<detailedSuspect>';
  var js2xmlopt = {'declaration':{'include':false}, 'prettyPrinting':{'enabled':false}};
  var classification = (Math.floor(parseFloat(suspect.GetClassification()) * 10000) / 100).toFixed(2);
  var d = new Date(suspect.GetLastPacketTime() * 1000);
  var dString = pad(d.getMonth() + 1) + "/" + pad(d.getDate()) + " " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds());
  var idinfoXmlTemplate = {'@':{'id':suspect.GetIdString(), 'ip':suspect.GetIpString(), 'iface':suspect.GetInterface(), 'class':classification, 'lpt':dString}};
  ret += j2xp('idInfo', idinfoXmlTemplate, js2xmlopt) + '>';
  var protoDataTemplate = {'@':{'tcp':suspect.GetTcpPacketCount(), 'udp':suspect.GetUdpPacketCount(), 'icmp':suspect.GetIcmpPacketCount(), 'other':suspect.GetOtherPacketCount()}};
  ret += j2xp('protoInfo', protoDataTemplate, js2xmlopt) + '>';
  var packetDataTemplate = {'@':{'rst':suspect.GetRstCount(), 'ack':suspect.GetAckCount(), 'syn':suspect.GetSynCount(), 'fin':suspect.GetFinCount(), 'synack':suspect.GetSynAckCount()}};
  ret += j2xp('packetInfo', packetDataTemplate, js2xmlopt) + '>';
  ret += '</detailedSuspect>';
  cb && cb(ret);
}

function pad(num)
{
  return ("0" + num.toString()).slice(-2);
}


