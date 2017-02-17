		setInterval("readTemp();", 2000);

		function loadJSON(callback) { 
			var xobj = new XMLHttpRequest();
			xobj.overrideMimeType("application/json");
			xobj.open('GET', 'data', true);
			xobj.onreadystatechange = function () {
				if (xobj.readyState == 4 && xobj.status == "200") {
					// .open will NOT return a value but simply returns undefined in async mode so use a callback
					callback(xobj.responseText);
				}
			}
			xobj.send(null);
		}
		
		function timeConverter(UNIX_timestamp){
		  var a = new Date(UNIX_timestamp * 1000);
		  var pre = 0;
		  var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
		  var year = a.getFullYear();
		  var month = months[a.getMonth()];
		  var date = a.getDate();
		  var hour = a.getHours() - "1";
		  var min = a.getMinutes();
		  var sec = a.getSeconds();
		  var time = date + ' ' + month + ' ' + year + ' ' + hour + ':' + min + ':' + sec ;
		  return time;
		}
		
		function readTemp(){
			loadJSON(function(response) {
				
			jsonresponse = JSON.parse(response);
			document.getElementById("time").innerHTML = timeConverter(jsonresponse.system.time);
			for (var i = 1; i <= 7; i++) {
				document.getElementById("name" + i).innerHTML = jsonresponse.channel[i - 1].name;
				document.getElementById("number" + i).innerHTML = "#" + jsonresponse.channel[i - 1].number;				
				// document.getElementById("typ" + i).innerHTML = jsonresponse.channel[i - 1].typ; 
				if (jsonresponse.channel[i - 1].temp < jsonresponse.channel[i - 1].min ) {
					document.getElementById("temp" + i).innerHTML = ((jsonresponse.channel[i - 1].temp == 999.00) ? "<font color='#2D2D2D'>&#xf2c9; </font>N.C.<sup>°C</sup>" : "<font color='#00B2EE'>&#xf2ca; </font> " + jsonresponse.channel[i - 1].temp + "<sup>°C</sup>");
				} else if (jsonresponse.channel[i - 1].temp > jsonresponse.channel[i - 1].max) {
					document.getElementById("temp" + i).innerHTML = ((jsonresponse.channel[i - 1].temp == 999.00) ? "<font color='#2D2D2D'>&#xf2c9; </font>N.C.<sup>°C</sup>" : "<font color='#ff4040'>&#xf2c7; </font>" + jsonresponse.channel[i - 1].temp + "<sup>°C</sup>");
				} else {
					document.getElementById("temp" + i).innerHTML = ((jsonresponse.channel[i - 1].temp == 999.00) ? "<font color='#2D2D2D'>&#xf2c9; </font>N.C.<sup>°C</sup>" : "<font color='#2D2D2D'>&#xf2c9; </font>" + jsonresponse.channel[i - 1].temp + "<sup>°C</sup>");
				}
				
				// document.getElementById("temp" + i).innerHTML = jsonresponse.channel[i - 1].temp; 
				document.getElementById("min" + i).innerHTML = "<font color='#00B2EE'>&#xf2ca; </font>" + jsonresponse.channel[i - 1].min + "°";
				document.getElementById("max" + i).innerHTML = "<font color='#ff4040'>&#xf2c7; </font>" + jsonresponse.channel[i - 1].max + "°"; 
				document.getElementById("set" + i).innerHTML = "<font color='#2D2D2D'>&#xf2c9; </font>" + jsonresponse.channel[i - 1].set + "°";
				// document.getElementById("alarm" + i).innerHTML = jsonresponse.channel[i - 1].alarm; 
				document.getElementById("ch" + i).style.borderColor = jsonresponse.channel[i - 1].color			
			}
		});
		}
		readTemp();
	if (!('boxShadow' in document.body.style)) {
    document.body.setAttribute('class', 'noBoxShadow');
}

document.body.addEventListener("click", function(e) {
    var target = e.target;
    if (target.tagName === "INPUT" &&
        target.getAttribute('class').indexOf('liga') === -1) {
        target.select();
    }
});

(function() {
    var fontSize = document.getElementById('fontSize'),
        testDrive = document.getElementById('testDrive'),
        testText = document.getElementById('testText');
    function updateTest() {
        testDrive.innerHTML = testText.value || String.fromCharCode(160);
        if (window.icomoonLiga) {
            window.icomoonLiga(testDrive);
        }
    }
    function updateSize() {
        testDrive.style.fontSize = fontSize.value + 'px';
    }
    fontSize.addEventListener('change', updateSize, false);
    testText.addEventListener('input', updateTest, false);
    testText.addEventListener('change', updateTest, false);
    updateSize();
}());
		