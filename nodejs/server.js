var http= require('http');
http.createServer(function(req,res)
{
    res.writeHead(200,{"Content-type":"text/blain"});
    res.write("Hellow NodeJs");
    res.end();
}).listen(8888);
