		setInterval("readTemp();", 2000);
	<!--
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
				
			// Do Something with the response e.g.
			jsonresponse = JSON.parse(response);
			document.getElementById("time").innerHTML = timeConverter(jsonresponse.system.time);
			for (var i = 0; i <= 6; i++) {
				document.getElementById("name" + i).innerHTML = jsonresponse.channel[i].name; 
				document.getElementById("typ" + i).innerHTML = jsonresponse.channel[i].typ; 
				document.getElementById("temp" + i).innerHTML = jsonresponse.channel[i].temp; 
				document.getElementById("min" + i).innerHTML = jsonresponse.channel[i].min;
				document.getElementById("max" + i).innerHTML = jsonresponse.channel[i].max; 
				document.getElementById("set" + i).innerHTML = jsonresponse.channel[i].set;
				document.getElementById("alarm" + i).innerHTML = jsonresponse.channel[i].alarm; 
				document.getElementById("ch" + i).style.borderColor = jsonresponse.channel[i].color			
			}
		});
		}
		readTemp();