	var en = {
				// Reiter System-Einstellungen
				system_settings:"System-Einstellungen", // System settings
				unit:"Unit",
				german:"German",
				english:"English",
				fahrenheit:"Fahrenheit",
				celsius:"Celsius",
				hostname:"Hostname",
				ap_name:"AP-Name",						// APN
				hw_version:"Hardwareversion",  			// Hardware version
				update_search:"Automatisch nach Updates suchen",  // Automatically check for updates
				// Buttons
				back:"Back",
				save:"Save",
				//Kanaleinstellungen
				channel_name:"Kanalname",		// Channel name
				temp_max:"Temp Max",			// Upper temperature limit
				temp_min:"Temp Min",			// Lower temperature limit
				temp_sensor:"Sensor",			// Probe type
				color:"Color",
				push_alarm:"Push Alarm",  		// Push alarm
				buzzer_alarm:"Summer Alarm",  	// Buzzer alarm
				//Menü
				menuHome:"Home",
				menuWlan:"WLAN",
				menuSystem:"System",
				menuPitmaster:"Pitmaster",
				menuIOT:"IoT",
				menuHistory:"History",
				menuNotification:"Push Notification",
				menuAbout:"About",
				//History
				btnSaveChart:"Save chart",
				historyTitle:"History",
				date:"Date",
				historyTableName:"Name",
				//Menü About
				aboutTitle:"WLANThermo Team",
				forumTitle:"WLANThermo Forum",
				forumUrl:"Wir freuen uns über deinen Besuch im Forum!",  // Visit us in our forum!
				//Menü Notification
				notificationTitle:"Benachrichtungsdienst",  // Notification service
				notificationActivate:"Push-Dienst aktivieren",	// Activate push notification
				notificationToken:"API Token/Key",
				notificationKey:"Chat ID / User Key",
				notificationService:"Dienst",				// Service
				repeadOnce:"1x",
				repeadThreeTimes:"3x",
				repeadFiveTimes:"5x",
				repeadTenTimes:"10x",
				notificationRepead:"Wiederholungen",		// Repeats
				notificationSendMessage:"Testnachricht senden",		// Send a test message
				//Cloud
				cloudTitle:"Nano Cloud",
				cloudActivate:"Nano Cloud aktivieren",  // Activate Nano Cloud
				tenSeconds:"10 Sekunden",			// 10 seconds
				fifteenSeconds:"15 Sekunden",		// 15 seconds
				thirtySeconds:"30 Sekunden",		// 30 seconds
				oneMinutes:"1 Minute",				// 1 minutes
				twoMinutes:"2 Minuten",				// 2 minutes
				fiveMinutes:"5 Minuten",			// 5 minutes  (wollen wir hier nur Zahlen machen und alles in Sekunden und dann Einheit zum Textfeld?)
				sendInterval:"Sendeintervall",   	// Send interval (in sec)
				cloudBtnToken:"Token generieren",	// Generate token
				mqttTitle:"Private MQTT Client",		
				mqttActivate:"Private MQTT aktivieren",  // Activate MQTT Client
				mqttHost:"MQTT Host",
				mqttPort:"MQTT Port",
				mqttUser:"MQTT User",
				mqttPwd:"MQTT Password",
				mqttQos:"QoS",
				// Menü WLAN
				wlanTitle:"WLAN-Einstellungen",		// WLAN settings
				wlanActivate:"WLAN aktivieren",		// Activate WLAN
				wlanNetwork:"Netzwerk wählen...",	// Select network
				wlanClear:"Netzwerkverbindungsdaten löschen",  // Delete stored network data
				wlanConnect:"Netzwerk Verbinden",	// Connect network
				wlanSSID:"SSID",
				wlanPwd:"Passwort",		// Password
				// Menü Pitmaster
				pitTitle:"Pitmaster-Einstellungen",  // Pitmaster settings
				pitPitmaster:"Pitmaster",
				pitProfile:"Profil",					// Profile
				pitChannel:"Kanal",						// Channel
				pitTemp:"Solltemperatur",				// Set temperature
				pitValue:"Pitmaster-Wert (%)",			// Pitmaster value (%)
				pitProfileName:"Profil Name",			// Profile name
				pitAktorTitle:"Aktorik",				// Actuator	
				pitAktor:"Aktor",						// Actuator
				pitDCmin:"DCmin [0-100%]",
				pitDCmax:"DCmax [0-100%]",
				pidTitle:"PID",
				pidKp:"Kp",
				pidKi:"Ki",
				pidKd:"Kd",
				pidJump:"Jump Power [10-100%]",
				pidAutotune:"Autotune",					// Auto tune
				pitAdvancesTitle:"Advanced",
				pitLid:"Deckelüberwachung"				// Lid monitoring
			};
	var de = {
				// Reiter System-Einstellungen
				system_settings:"System-Einstellungen",
				unit:"Einheit",		
				german:"Deutsch",
				english:"Englisch",
				fahrenheit:"Fahrenheit",
				celsius:"Celsius",
				hostname:"Hostname",			// warum 2x?
				hostname:"Hostname",
				ap_name:"AP-Name",				// APN
				hw_version:"Hardwareversion",
				update_search:"Automatisch nach Updates suchen",
				// Buttons
				back:"Zurück",
				save:"Speichern",
				//Kanaleinstellungen
				channel_name:"Kanalname",
				temp_max:"Temp Max",			// Temperatur-Obergrenze
				temp_min:"Temp Min",			// Temperatur-Untergrenze
				temp_sensor:"Fühler",			// Fühlertyp
				color:"Farbe",
				push_alarm:"Push Alarm",		// Push-Alarm
				buzzer_alarm:"Summer Alarm",	// Piepser-Alarm
				//Menü
				menuHome:"Home",
				menuWlan:"WLAN",
				menuSystem:"System",
				menuPitmaster:"Pitmaster",
				menuIOT:"IoT",
				menuHistory:"History",					// Historie   was denkst du?
				menuNotification:"Push Notification",	// Benachrichtigung
				menuAbout:"Über",
				//History
				btnSaveChart:"Chart Speichern",			// Chart speichern
				historyTitle:"History",					// Historie
				date:"Datum",
				historyTableName:"Name",
				//Menü About
				aboutTitle:"WLANThermo Team",
				forumTitle:"WLANThermo Forum",
				forumUrl:"Wir freuen uns über deinen Besuch im Forum!",
				//Menü Notification
				notificationTitle:"Benachrichtungsdienst",
				notificationActivate:"Push-Dienst aktivieren",  // Push-Benachrichtung aktivieren
				notificationToken:"API Token/Key",
				notificationKey:"Chat ID / User Key",
				notificationService:"Dienst",				
				repeadOnce:"1x",
				repeadThreeTimes:"3x",
				repeadFiveTimes:"5x",
				repeadTenTimes:"10x",
				notificationRepead:"Wiederholungen",
				notificationSendMessage:"Testnachricht senden",
				//Cloud
				cloudTitle:"Nano Cloud",				// Nano-Cloud
				cloudActivate:"Nano Cloud aktivieren",	// Nano-Cloud aktivieren
				tenSeconds:"10 Sekunden",				// siehe Hinweis EN
				fifteenSeconds:"15 Sekunden",			
				thirtySeconds:"30 Sekunden",
				oneMinutes:"1 Minute",
				twoMinutes:"2 Minuten",
				fiveMinutes:"5 Minuten",
				sendInterval:"Sendeintervall",			// Sendeintervall (in Sek.)
				cloudBtnToken:"Token generieren",
				mqttTitle:"Private MQTT Client",		// Privater MQTT Client
				mqttActivate:"Private MQTT aktivieren", // MQTT Client aktivieren
				mqttHost:"MQTT Host",
				mqttPort:"MQTT Port",
				mqttUser:"MQTT User",					// MQTT Benutzer
				mqttPwd:"MQTT Password",				// MQTT Passwort
				mqttQos:"QoS",
				// Menü WLAN
				wlanTitle:"WLAN-Einstellungen",
				wlanActivate:"WLAN aktivieren",
				wlanNetwork:"Netzwerk wählen...",
				wlanClear:"Netzwerkverbindungsdaten löschen",
				wlanConnect:"Netzwerk Verbinden",			// Netzwerk verbinden
				wlanSSID:"SSID",
				wlanPwd:"Passwort",
				// Menü Pitmaster
				pitTitle:"Pitmaster-Einstellungen",
				pitPitmaster:"Pitmaster",
				pitProfile:"Profil",
				pitChannel:"Kanal",
				pitTemp:"Solltemperatur",
				pitValue:"Pitmaster-Wert (%)",
				pitProfileName:"Profil Name",			// Profil-Name
				pitAktorTitle:"Aktorik",
				pitAktor:"Aktor",
				pitDCmin:"DCmin [0-100%]",
				pitDCmax:"DCmax [0-100%]",
				pidTitle:"PID",
				pidKp:"Kp",
				pidKi:"Ki",
				pidKd:"Kd",
				pidJump:"Jump Power [10-100%]",					
				pidAutotune:"Autotune",
				pitAdvancesTitle:"Advanced",			// Spezialfunktionen		
				pitLid:"Deckelüberwachung"					
			};