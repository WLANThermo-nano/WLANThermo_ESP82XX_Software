(function (window, document) {

    var layout   = document.getElementById('layout'),
        menu     = document.getElementById('menu'),
        menuLink = document.getElementById('menuLink'),
		menuHome = document.getElementById('menuHome'),
		menuPlot = document.getElementById('menuPlot'),
		menuWlan = document.getElementById('menuWlan'),
		menuSystem = document.getElementById('menuSystem'),
		menuPitmaster = document.getElementById('menuPitmaster'),
		menuChart = document.getElementById('menuChart'),
		menuChannel1 = document.getElementById('menuChannel1'),
		menuChannel2 = document.getElementById('menuChannel2'),
		menuChannel3 = document.getElementById('menuChannel3'),
		menuChannel4 = document.getElementById('menuChannel4'),
		menuChannel5 = document.getElementById('menuChannel5'),
		menuChannel6 = document.getElementById('menuChannel6'),
		menuChannel7 = document.getElementById('menuChannel7'),
		menuChannel8 = document.getElementById('menuChannel8'),
		menuAbout = document.getElementById('menuAbout'),
        content  = document.getElementById('main');
    function toggleClass(element, className) {
        var classes = element.className.split(/\s+/),
            length = classes.length,
            i = 0;

        for(; i < length; i++) {
          if (classes[i] === className) {
            classes.splice(i, 1);
            break;
          }
        }
        // The className is not found
        if (length === classes.length) {
            classes.push(className);
        }

        element.className = classes.join(' ');
    }

    function toggleAll(e) {
        var active = 'active';
        e.preventDefault();
        toggleClass(layout, active);
        toggleClass(menu, active);
        toggleClass(menuLink, active);
    }

    menuLink.onclick = function (e) {
        toggleAll(e);
    };
    menuHome.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showIndex();
        }
    };
    menuPlot.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showPlot();
        }
    };
    menuWlan.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showWLAN();
        }
    };
    menuSystem.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSystem();
        }
    };
    menuPitmaster.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showPitmaster();
        }
    };
    menuChart.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showChart();
        }
    };
    menuChannel1.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('1');
        }
    };
	menuChannel2.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('1');
        }
    };
	menuChannel3.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('3');
        }
    };
	menuChannel4.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('4');
        }
    };
	menuChannel5.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('5');
        }
    };
	menuChannel6.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('6');
        }
    };
	menuChannel7.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('7');
        }
    };
	menuChannel7.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showSetChannel('8');
        }
    };
    menuAbout.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
			showAbout();
        }
    };
 
    content.onclick = function(e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
        }
    };

}(this, this.document));