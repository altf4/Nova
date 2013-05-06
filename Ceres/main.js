var https = require('https');
var http = require('http');
var fs = require('fs');
var url = require('url');

var options = {
  // TODO: These need to be gotten from Config variables
  key:  fs.readFileSync('keys/privatekey.pem'),
  cert: fs.readFileSync('keys/certificate.pem')
};

var handler = function(req, res) {
  var reqSwitch = url.parse(req.url).pathname;
  var params = url.parse(req.url, true).query;
  
  console.log('reqSwitch == ' + reqSwitch);
  for(var i in params)
  {
    console.log('params[' + i + '] == ' + params[i]);
  }
  
  switch(reqSwitch)
  {
    case '/getAll': res.writeHead(200, {'Content-Type':'text/plain'});
              res.end('Hello World\n');
              break;
    default:  res.writeHead(404);
              res.end('404, yo\n');
              break;
  }
};

var server = https.createServer(options, handler).listen(8443);

