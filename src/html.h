const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    img { 
      display: inline-block;
      vertical-align: middle;
      max-height: 100%;
      max-width: 100%;
    }
  </style>
</head>
<body>
  <h2>ESP32 Thermal Camera</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature: </span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup><br>
    
    <span class="dht-labels">Temp Max: </span> 
    <span id="tempmax">%TEMPMAX%</span>
    <sup class="units">&deg;C</sup><br>
    
    <span class="dht-labels">Temp Min: </span> 
    <span id="tempmin">%TEMPMIN%</span>
    <sup class="units">&deg;C</sup>

    <img src="thermal" id="thermalimage" style="width: 100%">
  </p>
</body>
<script>

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("tempmax").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/tempmax", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("tempmin").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/tempmin", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    //if (this.readyState == 4 && this.status == 200) {
      var image = document.getElementById("thermalimage");
      //image.src = this.response;
      image.src = "thermal";
      //document.getElementById("thermal") = this.response;
    //}
  };
  xhttp.open("GET", "/thermal", true);
  xhttp.send();
}, 1000 ) ;

</script>
</html>)rawliteral";
