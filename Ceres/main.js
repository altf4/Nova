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
var NovaCommon = require('../Quasar/NovaCommon.js');
var https = require('https');
var http = require('http');
var fs = require('fs');
var j2xp = require('js2xmlparser');
var url = require('url');
var NovaHomePath = NovaCommon.config.GetPathHome();

var options = {
  // TODO: These need to be gotten from Config variables
  key:  fs.readFileSync('keys/privatekey.pem'),
  cert: fs.readFileSync('keys/certificate.pem')
};

NovaCommon.nova.CheckConnection();
NovaCommon.StartNovad(false);
ReloadInterfaceAliasFile();

var handler = function(req, res) {
  var reqSwitch = url.parse(req.url).pathname;
  var params = url.parse(req.url, true).query;

  switch(reqSwitch)
  {
    case '/getAll': 
              res.writeHead(200, {'Content-Type':'text/plain'});
              NovaCommon.nova.CheckConnection();
              setTimeout(function(){
                NovaCommon.GetSuspects(-1, 0, "classification", "DESC", true, function(err, suspects){
                  if(err) {
                    console.log("Error fetching suspects: " + err);
                    return;
                  }
                  gridPageSuspectList(suspects, function(xml){
                    res.end(xml);
                  });
                });
              }, 2000);
              break;
      case '/getSuspect':
              res.writeHead(200, {'Content-Type':'text/plain'});
              var ipiface = params.suspect.split(':');
              NovaCommon.nova.CheckConnection();
              setTimeout(function(){
                NovaCommon.dbqGetSuspect.all(ipiface[0], ipiface[1], function(err, suspect){
                  if (err) {
                    console.log("ERROR: " + err);
                    return;
                  }

                  if (suspect.length != 1) {
                    console.log("ERROR: could not fetch suspect " + ipiface[0] + "/" + ipiface[1]);
                    return;
                  }

                  detailedSuspectPage(suspect[0], function(xml){
                    res.end(xml);
                  });
                });
              }, 2000);
              break;
    default:  res.writeHead(404);
              res.end('404, yo\n');
              break;
  }
};

var server = https.createServer(options, handler).listen(8443);

function ReloadInterfaceAliasFile() 
{
    var aliasFileData = fs.readFileSync(NovaHomePath + "/config/interface_aliases.txt");
    interfaceAliases = JSON.parse(aliasFileData);
}

function ConvertInterfaceToAlias(iface) 
{
    if(interfaceAliases[iface] !== undefined) 
    {
        return interfaceAliases[iface];
    } 
    else 
    {
        return iface;
    }
}

function gridPageSuspectList(suspects, cb)
{
  var suspectRet = '<suspects>';
  var js2xmlopt = {'declaration':{'include':false}, 'prettyPrinting':{'enabled':false}};
  for(var i in suspects)
  {
    var ip = suspects[i].ip;
    var iface = ConvertInterfaceToAlias(suspects[i].interface);
    var classification = (Math.floor(parseFloat(suspects[i].classification) * 10000) / 100).toFixed(2);
    var hostile = (suspects[i].isHostile == true ? '1' : '0');
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
  var classification = (Math.floor(parseFloat(suspect.classification) * 10000) / 100).toFixed(2);
  var d = new Date(suspect.lastTime * 1000);
  var dString = pad(d.getMonth() + 1) + "/" + pad(d.getDate()) + " " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds());
  var idinfoXmlTemplate = {'@':{'id':suspect.interface + " " + suspect.ip, 'ip':suspect.ip, 'iface':suspect.interface, 'class':classification, 'lpt':dString}};
  ret += j2xp('idInfo', idinfoXmlTemplate, js2xmlopt) + '>';
  var protoDataTemplate = {'@':{'tcp':suspect.count_tcp, 'udp':suspect.count_udp, 'icmp':suspect.count_icmp, 'other':suspect.count_other}};
  ret += j2xp('protoInfo', protoDataTemplate, js2xmlopt) + '>';
  var packetDataTemplate = {'@':{'rst':suspect.count_tcpRst, 'ack':suspect.count_tcpAck, 'syn':suspect.count_tcpSyn, 'fin':suspect.count_tcpFin, 'synack':suspect.count_tcpSynAck}};

  ret += j2xp('packetInfo', packetDataTemplate, js2xmlopt) + '>';
  ret += '</detailedSuspect>';
  cb && cb(ret);
}

function pad(num)
{
  return ("0" + num.toString()).slice(-2);
}
