var express = require('express');
var app = express();
var path = require('path');
var bodyParser = require('body-parser');
var fs = require('fs');

var jsonParser = bodyParser.json();

app.get('/', function (req, res) {
    res.sendFile(path.join(__dirname, "/index.html"));
});

// example response
/* 
{
    testResult: {
      latency: 30,
      jitter: 39,
      download: 408.69,
      upload: 162.06,
      maxDownload: 415.55,
      maxUpload: 211.51,
      testServer: 'San Jose 5',
      ip_address: '162.237.77.152',
      hostname: '162-237-77-152.lightspeed.sntcca.sbcglobal.net',
      userAgent: 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:124.0) Gecko/20100101 Firefox/124.0',
      testDate: '2024-04-28T22:55:52.055Z'
    }
} 
*/
  
app.post('/results', jsonParser, function (req, res) {
    console.log("getting results from online");
//    console.log(req.body);
    var currentDate = new Date();
    fs.writeFile("/home/client/results/speedofme/results_" + currentDate + ".txt", JSON.stringify(req.body), err => {
        if (err) throw err;
        console.log("file written");
    })
    res.sendStatus(200, "good res");
})

app.listen(3000, function() {
    console.log("app listening on 3000");
});
