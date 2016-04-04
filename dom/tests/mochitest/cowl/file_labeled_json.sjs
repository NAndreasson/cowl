function handleRequest(request, response) {
  //Allow cross-origin, so you can XHR to it!
  response.setHeader("Access-Control-Allow-Origin", "*", false);
  response.setHeader("Content-Type", "application/labeled-json", false);

  // Get some scenario id, then do different stuff depending on the case...
  var labeledObject = {
    confidentiality: "'self'",
    integrity: "'self'",
    object: {
      val: "secret"
    }
  };

  response.write(JSON.stringify(labeledObject));
}
