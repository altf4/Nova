var novaconfig = require('novaconfig.node');

var nova = new novaconfig.Instance();
var config = new novaconfig.NovaConfigBinding();

var honeydConfig = new novaconfig.HoneydConfigBinding();
honeydConfig.LoadAllTemplates();

var vendorToMacDb = new novaconfig.VendorMacDbBinding();
var osPersonalityDb = new novaconfig.OsPersonalityDbBinding();
var trainingDb = new novaconfig.CustomizeTrainingBinding();
var whitelistConfig = new novaconfig.WhitelistConfigurationBinding();

var assert = require("assert");

describe('honeydConfig', function(){
	describe('#GetProfileNames()', function(){
		it('should return default configuration profile names', function(){
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("default"));
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("LinuxServer"));
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("WinServer"));
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("BSDServer"));

			assert.equal(-1, honeydConfig.GetProfileNames().indexOf("non-existant fake profile name"));
		})
	})	

	describe('#GetScript()', function(){
		it('should return the Linux httpd script', function(){
			var script = honeydConfig.GetScript("Linux httpd");
			assert.equal("Linux httpd", script.GetName());
			assert.equal("http", script.GetService());
			assert.equal("Linux | Linux", script.GetOsClass());
			assert.equal("tclsh /usr/share/honeyd/scripts/linux/httpd/httpd.tcl $ipsrc $sport $ipdst $dport", script.GetPath());
			assert(script.GetIsConfigurable());
		
			var options = script.GetOptions();
			assert(options["HTTPD_RESPONSE_FOLDER"] !== undefined);
			assert(options["HTTPD_SERVER_VERSION"] !== undefined);
			assert(options["HTTPD_SERVER_VERSION"].indexOf("Apache") !== -1);
		
		})
	})

})
