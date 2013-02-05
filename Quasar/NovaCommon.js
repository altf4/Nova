//============================================================================
// Name        : NovaCommon.js
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
// Description : Common code for Nova modules
//============================================================================

var NovaCommon = new function() {
    console.log("Initializing nova C++ code");
    this.novaconfig = require('novaconfig.node');
    this.nova = new this.novaconfig.Instance();
    this.config = new this.novaconfig.NovaConfigBinding();
    this.honeydConfig = new this.novaconfig.HoneydConfigBinding();
    this.vendorToMacDb = new this.novaconfig.VendorMacDbBinding();
    this.osPersonalityDb = new this.novaconfig.OsPersonalityDbBinding();
    this.trainingDb = new this.novaconfig.CustomizeTrainingBinding();
    this.whitelistConfig = new this.novaconfig.WhitelistConfigurationBinding();
    this.LOG = require("../NodejsModule/Javascript/Logger").LOG;

    var classifiersConstructor = new require('./classifiers.js');
    this.classifiers = new classifiersConstructor(this.config);

    this.cNodeToJs = function(node)
    {
        var ret = {};
        ret.enabled = node.IsEnabled();
        ret.pfile = node.GetProfile();
        ret.ip = node.GetIP();
        ret.mac = node.GetMAC();
        ret.interface = node.GetInterface();
        return ret;
    }

    this.GetPorts = function()
    {
      var scriptBindings = {};
      
      var profiles = this.honeydConfig.GetProfileNames();
      
      for(var i in profiles)
      {
        var profileName = profiles[i];
    
        var portSets = this.honeydConfig.GetPortSetNames(profiles[i]);
        for (var portSetName in portSets)
        {
            var portSet = this.honeydConfig.GetPortSet(profiles[i], portSets[portSetName]);
            var ports = [];
            for (var p in portSet.GetTCPPorts())
            {
                ports.push(portSet.GetTCPPorts()[p]);
            }
            for (var p in portSet.GetUDPPorts())
            {
                ports.push(portSet.GetUDPPorts()[p]);
            }
            for(var p in ports)
            {
                if(ports[p].GetScriptName() != undefined && ports[p].GetScriptName() != '')
                {
                    if(scriptBindings[ports[p].GetScriptName()] == undefined)
                    {
                        scriptBindings[ports[p].GetScriptName()] = profileName + "(" + portSet.GetName() + ")" + ':' + ports[p].GetPortNum();
                    } else {
                        scriptBindings[ports[p].GetScriptName()] += '<br>' + profileName + "(" + portSet.GetName() + ")" + ':' + ports[p].GetPortNum();
                    }
                }
            }
        }
      }
    
      return scriptBindings;
    }
}();

module.exports = NovaCommon;
