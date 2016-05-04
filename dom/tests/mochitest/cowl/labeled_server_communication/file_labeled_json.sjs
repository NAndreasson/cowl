function handleRequest(request, response) {
  //Allow cross-origin, so you can XHR to it!
  response.setHeader("Access-Control-Allow-Origin", "*", false);
  response.setHeader("Content-Type", "application/labeled-json", false);

  var query = request.queryString.split("=")[1];

  // Get some scenario id, then do different stuff depending on the case...
  var labeledObject = {
    confidentiality: "'self'",
    integrity: "'self'",
    object: {
      val: "secret"
    }
  };

  if (query == 'conf') {
    delete labeledObject['confidentiality'];
  }
  if (query == 'int') {
    delete labeledObject['confidentiality'];
  }
  if (query == 'obj') {
    delete labeledObject['confidentiality'];
  }
  if (query == 'badint') {
    labeledObject['integrity'] = 'http://a.com/'; // should be served from example.com so should not be valid
  }

  response.write(JSON.stringify(labeledObject));
}
