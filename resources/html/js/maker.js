/**
 * global objects placeholder
 */
var global = {menu_id:0, menu: [], value:{}};


/**
 * EmbUI's js api version
 * used to set compatibilty dependency between backend firmware and WebUI js
 */
const ui_jsapi = 5;

/**
 * User application versions - frontend/backend
 * could be overriden with user js code to make EmbUI check code compatibilty
 * checked as integer comparison if not zero
 */
var app_jsapi = 0;

/**
 * An object with loadable UI blocks that could be stored on client's side
 * blocks could be requested to render on-demand from the backend side, so it could
 * save backend from generating block objects every time and sending it to Front-End 
 */
var uiblocks = {};

/**
 * callback for unknown pkg types - Stub function to handle user-defined types messages.
 * Data could be sent from the controller with any 'pkg' types except internal ones used by EmnUI
 * and handled in a user-define js function via reassigning this value
 */
var unknown_pkg_callback = function (msg){
    console.log('Got unknown pkg data, redefine "unknown_pkg_callback" to handle it.', msg);
}


/**
 * A placeholder array for user js-functions that could be executed on button click
 * any user-specific funcs could be appended/removed/replaced to this array later on
 * 
 * UI method to create interactive button is
 * Interface::button_js(const String &id, const String &label, const String &color = "", const T &value = nullptr)
 * where:
 *  'id' is function selector,
 * 	'value' is passed value
 */
var customFuncs = {
	// BasicUI class - set system date/time from the browser
	set_time: function (event) {
		var t = new Date();
		var tLocal = new Date(t - t.getTimezoneOffset() * 60 * 1000);
		var isodate = tLocal.toISOString().slice(0, 19);	// chop off chars after ss

		var data = {};
		let value = event.target.value;
		if (value){
			// if there was a param 'value', then paste the date string into doc element with id='value'
			// let's do this via simulating MCU value frame
			console.log("Set specific date:", value)
			data["block"] = [];
			data.block.push({"date" : value});
			var r = render();
			r.value(data);
		} else {
			// if there was no 'value' given, than simply post browser's date string to MCU that triggers time/date setup
			console.log("Set browser's date:", isodate)
			data["set_sys_datetime"] = isodate;
			ws.send_post("set_sys_datetime", data);
		}
	}//,
	//func2: function (event) {     console.log('Called func 2'); }
};

/**
 * User Callback for xload() function. Вызывается после завершения загрузки внешних данных, но
 * перед предачей объекта обработчику шаблона. Если коллбек возвращает false, то вызов шаблонизатора не происходит.
 * Может быть перенакрыта пользовательским скриптом для доп обработки загруженных данных
 */
function xload_cb(obj){
//    console.log('redefine xload_cb(obj) func to handle it.', msg);
	return true;
}

/**
 * @brief - Loads JSON objects via http request
 * @param {*} url - URI to load
 * @param {*} ok - callback on success
 * @param {*} err - callback on error
 */
function ajaxload(url, ok, err){
	var xhr = new XMLHttpRequest();
	xhr.overrideMimeType("application/json");
	xhr.responseType = 'json';
	xhr.open('GET', url, true);
	xhr.onreadystatechange = function(){
		if (xhr.readyState == 4 && xhr.status == "200") {
			ok && ok(xhr.response);
		} else if (xhr.status != "200"){
			err && err(xhr.status)
		}
	};
	xhr.send(null);
}

/**
 * 	"pkg":"xload" messages are used to make ajax requests for external JSON objects that could be used as data/interface templates
 * используется для загрузки контента/шаблонов из внешних источников - "флеш" контроллера, интернет ресурсы с погодой и т.п.,
 * объекты должны сохранять структуру как если бы они пришли от контроллера. Просмариваются рекурсивно все секции у которых есть ключ 'url',
 * этот урл запрашивается и результат записывается в ключ 'block' текущей секции. Ожидается что по URL будет доступен корректный JSON.
 * Результат передается в рендерер и встраивается в страницу /Vortigont/
 * @param { * } msg - framework content/interface object
 * @returns 
 */
function xload(msg){
    if (!msg.block){
        console.log('Message has no data block!');
        return;
    }

	console.log('Run deepfetch');
	deepfetch(msg.block).then(() => {
		 //console.log(msg);
		 if (xload_cb(msg)){
			var rdr = this.rdr = render();	// Interface rederer to pass updated objects to
			rdr.make(msg);
		 }
	})
}

/**
 * async function to traverse object and fetch all 'url' in sections,
 * this must be done in async mode till the last end, since there could be multiple recursive ajax requests
 * including in sections that were just fetched /Vortigont/
 * @param {*} obj - EmbUI data object
 */
async function deepfetch (obj) {
	for (let i = 0; i < obj.length; i++) if (typeof obj[i] == "object") {
		let element = obj[i];
		if (element.url){
			console.log('xload URL: ' + element.url);
			await new Promise(next=> {
					ajaxload(element.url,
						function(response) {
							element['block'] = response;
							delete element.url;	// удаляем url из элемента т.к. работает рекурсия
							// пробегаемся рекурсивно по новым/вложенным объектам
							if (element.block && typeof element.block == "object") {
								deepfetch(element.block).then(() => {
									next();
							   })
							} else {
								next();
							}
						},
						function(errstatus) {
							next();
						}
					);
			})
		} else if ( element.block && typeof element.block == "object" ){
			await new Promise(next=> {
				deepfetch(element.block).then(() => {
					next();
				})
			})
		}
	}
}

/**
 * this function recoursevily looks through sections and picks ones with name 'xload'
 * for such sections all html elements with 'url' object key will be side-load with object content into
 * and object named 'block'. Same way as for xload() function.
 * @param {*} msg - object with UI sections
 * @returns 
 */
async function section_xload(msg){
    if (!msg.block) return;
	for (let i = 0; i < msg.block.length; i++){
		let el = msg.block[i];
		if (msg.section == "xload" && el.url){
			// do side load for all elements of the section
			console.log('section xload URL: ', el.url);
			await new Promise(next=> {
				ajaxload(el.url,
					function(response) {
						el['block'] = response;
						next();
					},
					function(errstatus) {
						next();
						//console.log('Error loading external content');
					}
				);
			})
		} else if (el.section){
			// recourse into nested secions
			await new Promise( next => { section_xload(el).then(() => { next(); }) } )
		}
	}
} 

/* Color gradients calculator. Source is from https://gist.github.com/joocer/bf1626d38dd74fef9d9e5fb18fef517c */
function colorGradient(colors, fadeFraction) {
	if (fadeFraction >= 1) {
		return colors[colors.length - 1]
	} else if (fadeFraction <= 0) {
		return colors[0]
	}

	var fade = fadeFraction * (colors.length - 1);
	var interval = Math.trunc(fade);
	fade = fade - interval;

	var color1 = colors[interval];
	var color2 = colors[interval + 1];

	var diffRed = color2.red - color1.red;
	var diffGreen = color2.green - color1.green;
	var diffBlue = color2.blue - color1.blue;

	var gradient = {
		red: parseInt(Math.floor(color1.red + (diffRed * fade)), 10),
		green: parseInt(Math.floor(color1.green + (diffGreen * fade)), 10),
		blue: parseInt(Math.floor(color1.blue + (diffBlue * fade)), 10),
	};

return 'rgb(' + gradient.red + ',' + gradient.green + ',' + gradient.blue + ')';
}

// a simple recursive iterator with callback
function recurseforeachkey(obj, f) {
	for (let key in obj) {
		if (typeof obj[key] === 'object') {
			if (Array.isArray(obj[key])) {
				for (let i = 0; i < obj[key].length; i++) {
				recurseforeachkey(obj[key][i], f);
				}
			} else {
				recurseforeachkey(obj[key], f);
			}
		} else {
		// run callback
		f(key, obj);
		}
	}
}
  

// template renderer
var render = function(){
	let chkNumeric = function(v){
		// cast empty strings to null
		if (typeof v == 'string' && v == "") return null;
		if(isFinite(v))
			return Number(v);
		else
			return v;
	};

	var tmpl_menu = new mustache(go("#tmpl_menu")[0],{
		on_page: function(d,id) {
			out.menu_change(id);
		}
	}),
	fn_section = {
		on_input: function(d, id){
			var value = this.value, type = this.type;
			if (type == "range") go("#"+id+"-val").html(": " + value);
			if (type == "text" || type == "password") go("#"+id+"-val").html(" ("+value.length+")");
			if (type == "color") go("#"+id+"-val").html(" ("+value+")");
			if (this.id != id){
				custom_hook(this.id, d, id);
			}
		},
		// handle dynamicaly changed elements on a page which should send value to WS server on each change
		on_change: function(d, id, val) {
			let value;

			// check if value has been supplied by templater
			if (val !== undefined){
				value = val;
			} else {
				switch (this.type){
					case 'checkbox':
						value = document.getElementById(id).checked;
						break;
					case 'number':
					case 'select-one':
					case 'range':
						value = chkNumeric(this.value);
						break;
					// cast empty strings to null in inputs
					case 'input':
					case 'textarea':
						value = (typeof this.value == 'string' && this.value == "") ? null : this.value;
					default:
						value = this.value;
				}
			}
			if (this.id != id){
				custom_hook(this.id, d, id);
			}
			let data = {};
			data[id] = value;

			ws.send_post(id, data);
		},
		on_showhide: function(d, id) {
			go("#"+id).showhide();
		},
		/**
		 *  Process Submited form
		 *  'submit' key defines action to be taken by the backend
		 */
		on_submit: function(d, id, val) {
			var form = go("#"+id), data = go.formdata(go("input, textarea, select", form));
			//data['submit'] = id;
			if (val !== undefined && typeof val !== 'object') { data[id] = val; }	// submit button _might_ have it's own value
			ws.send_post(id, data);
		},
		// run custom user-js function
		on_js: function(event, callback) {
			if (callback in customFuncs)
				customFuncs[callback](event);
			else
				console.log("User function undefined: ", callback);
		}
	},
	tmpl_section = new mustache(go("#tmpl_section")[0], fn_section),
	tmpl_section_main = new mustache(go("#tmpl_section_main")[0], fn_section),
	tmpl_content = new mustache(go("#tmpl_content")[0], fn_section),
	out = {
		lockhist: false,
		history: function(hist){
			history.pushState({hist:hist}, '', '?'+hist);
		},
		menu_change: function(menu_id){
			global.menu_id = menu_id;
			this.menu();
			go("#main > div").display("none");
			// go("#main #"+menu_id).display("block");
			// var data = {}; data[menu_id] = null
			ws.send_post(menu_id, {});
		},
		menu: function(){
			go("#menu").clear().append(tmpl_menu.parse(global));
		},
		section: function(obj){
			if (obj.main) {
				go("#main > section").remove();
				go("#main").append(tmpl_section_main.parse(obj));
				if (!out.lockhist) out.history(obj.section);
			} else {
				if ( Object.keys(go("#"+obj.section)).length === 0 && !obj.replace ){
					go("#main").append(tmpl_section_main.parse(obj));
				} else {
					console.log("replacing section:", obj.section)
					go("#"+obj.section).replace(tmpl_section.parse(obj));
				}
			}
		},
		// process "pkg":"interface" messages and render template
		make: function(obj){
			if (!obj.block) return;
			let frame = obj.block;

			function recourseUIData(arr){
				for (let i = 0; i != arr.length; ++i){
					if (typeof arr[i] != "object") continue;
					if(arr[i].section == "uidata" && arr[i].block.length){
						let newblocks = []	// an array for sideloaded blocks
						arr[i].block.forEach(async function(v, idx, array){
							// Load UI data objects from external resource, i.e. json file
							if(v.action == "xload"){
								let response = await fetch(v.url, {method: 'GET'});
								if (!response.ok) return;
								response = await response.json();
								if(v.merge)
									_.merge(_.get(uiblocks, v.key), response)
								else
									_.set(uiblocks, v.key, response);
								//console.log("loaded uiobj:", response, " ver:", response.version);
								// check if loaded data is older then requested from backend
								//console.log("Req ver: ", v.version, " vs loaded ver:", _.get(uiblocks, v.key).version)
								if (v.version > _.get(uiblocks, v.key).version){
									console.log("Opening update alert msg");
									document.getElementById("update_alert").style.display = "block";
								}
								//return
							}
							if(v.action == "xmerge"){
								let response = await fetch(v.url, {method: 'GET'});
								if (!response.ok) return;
								response = await response.json();
								if(v.src)
									_.merge(_.get(uiblocks, v.key), _.get(response, v.src))
								else
									_.merge(_.get(uiblocks, v.key), response)
								//console.log("merge uiobj under:", v.key, "src:", v.src, "data:", response);
								return
							}
							// Pick UI object from a previously loaded UI data storage
							if (v.action == "pick"){
								//console.log("pick obj:", v.key);
								let ui_obj = {};
								if (v.prefix || v.suffix)
									// make a deep copy, 'cause we'll modify the object
									ui_obj = JSON.parse(JSON.stringify(_.get(uiblocks, v.key)));
								else
									ui_obj = _.get(uiblocks, v.key)
								if (Object.keys(ui_obj).length !== 0){
									if (v.prefix){
										ui_obj.section = v.prefix + obj[key];
										recurseforeachkey(ui_obj, function(key, obj){ if (key === "id"){ obj[key] = v.prefix + obj[key]; } })
									}
									if (v.suffix){
										ui_obj.section += v.suffix;
										recurseforeachkey(ui_obj, function(key, obj){ if (key === "id"){ obj[key] += v.suffix; } })
									}
									newblocks.push(ui_obj)
								}
								//return
							}
						})
						// a function that will replace current section item with uidata items
						function implaceArrayAt(array, idx, arrayToInsert) {
							Array.prototype.splice.apply(array, [idx, 1].concat(arrayToInsert));
							return array;
						}
						if (newblocks.length)
							implaceArrayAt(arr, i, newblocks)
						else
							arr.splice(i, 1)
						// since array has changed, I must reiterate it
						recourseUIData(arr)
						break
					}

					// dive into nested section
					if (arr[i].section && arr[i].block.length)
						recourseUIData(arr[i].block)
				}
			}

			// deep iterate and process "uidata" sections
			recourseUIData(frame);
			console.log("Processed packet:", obj);

			// go through 1-st level section and render it according to type of data
			for (let i = 0; i < frame.length; i++) if (typeof frame[i] == "object") {
				if (frame[i].section == "content") {
					for (let n = 0; n < frame[i].block.length; n++) {
						go("#"+frame[i].block[n].id).replace(tmpl_content.parse(frame[i].block[n]));
					}
					continue;
				}
				if (frame[i].section == "manifest"){
					let manifest = frame[i];
					document.title = manifest.app + " - " + manifest.mc;
					global.app = manifest.app;
					global.macid = manifest.mc;
					global.uiver = manifest.uiver;
					global.uijsapi = manifest.uijsapi;
					global.uiobjects = manifest.uiobjects;
					global.appver = manifest.appver;
					global.appjsapi = manifest.appjsapi;
					if (global.uijsapi > ui_jsapi || global.appjsapi > app_jsapi || global.uiobjects > uiblocks.sys.version)
						document.getElementById("update_alert").style.display = "block";
					continue;
				}
				if (frame[i].section == "menu"){
					global.menu =  frame[i].block;
					if (!global.menu_id) global.menu_id = global.menu[0].value
					this.menu();
					continue;
				}
				// look for nested xload sections and await for side-load, if found
				section_xload(frame[i]).then( () => { this.section(frame[i]); } );
			}
			out.lockhist = false;
		},
		// processing packets with values - "pkg":"value"
		// блок разбирается на объекты по id и их value применяются к элементам шаблона на html странице
		value: function(obj){
			let frame = obj.block;
			if (!obj.block) return;

			/*
				Find DOM object with id 'key' and sets it's 'value' property to 'val'.
				If 'html' is set to 'true', than values applied as html-text value,
				i.e. template tags visible on the page  ( <span>{{value}}</span> )
				otherwise value applies to an html element's attribute ( <input type="range" value="{{value}}" )
			*/
			function setValue(key, val, html = false){
				if (val == null || typeof val == "object") return;	// skip undef/null or (empty) objects
				let el = go("#"+key);
				if (!el.length) return;
				if (html === true ){ el.html(val); return; }

				// checkbox state
				if (el[0].type == "checkbox") {
					// allow multiple types of TRUE value for checkboxes
					el[0].checked = (val == true  ||  val == 1 || val == "1" || val == "true" );
					return;
				}

				// update progressbar's
				if (el[0].className == "progressab") {
					// value of '0' is endless 'in-progress'
					if (val == 0){
						el[0].style.width = "100%";
						el[0].innerText = "...";
						el[0].style.backgroundColor = 'rgb(10,0,0)';
						return;
					}

					el[0].style.width = val+"%";
					el[0].innerText = val+"% Complete";

					var color1 = { red: 200, green: 0, blue: 0 };
					var color2 = { red: 220, green: 204, blue: 0 };
					var color3 = { red: 0, green: 200, blue: 0 };
					var color4 = { red: 0, green: 0, blue: 240 };
					var bar_color = colorGradient([ color1, color2, color3, color4 ], val/100 ); // expect percents here
					el[0].style.backgroundColor = bar_color;
					//console.log("progress upd ", el[0], " color: ", bar_color);
					return;
				}

				if (el[0].type == "range") { go("#"+el[0].id+"-val").html(": "+val); }		// update span with range's label

				//console.log("Element is: ", el[0], " class is: ", el[0].className);
				el[0].value = val;
			}

			for (var i = 0; i < frame.length; i++) if (typeof frame[i] == "object") {
				// check if the object contains just an array with key:value pairs (comes from echo-back packets)
				if ('id' in frame[i] && 'value' in frame[i]){
					/* otherwise it must be a dict
						{ "id": "someid", "value": "somevalue", "html": true }
					*/
					setValue(frame[i].id, frame[i].value, frame[i].html);
				} else {
					for(let k in frame[i]) {
						setValue(k, frame[i][k]);
					}
				}
			}
		}
	};
	return out;
};



window.addEventListener("load", async function(ev){
	var rdr = this.rdr = render();
	var ws = this.ws = wbs("ws://"+location.host+"/ws");

	ws.oninterface = function(msg) { rdr.make(msg) }

	// run any js function in window context
	ws.onjscall = function(msg) {
		if ( msg.jsfunc && typeof window[msg.jsfunc] == "function"){
			try { 	console.log("JSCall: ", msg.jsfunc);
					window[msg.jsfunc](msg);
				} catch(e){ console.log('Error on calling function:'+msg.function, e); }
		};
	}

	ws.onvalue = function(msg){ rdr.value(msg) }
	ws.onclose = ws.onerror = function(){ ws.connect() }

	// "pkg":"xload" messages are used to make ajax requests for external JSON objects that could be used as data/interface templates
	ws.onxload = function(mgs){ xload(mgs) }

	// any Packets with unknown "pkg":"type" are handled here via user-redefinable callback
	ws.onUnknown = function(msg){ unknown_pkg_callback(msg) }

	// load sys UI objects
	let response = await fetch("/js/ui_sys.json", {method: 'GET'});
	if (response.ok){
		response = await response.json();
		uiblocks['sys'] = response;
		//console.log("loaded obj:", uiblocks.sys);
	}

	ws.connect();

	var active = false, layout =  go("#layout");
	go("#menuLink").bind("click", function(){
		active = !active;
		if (active) layout.addClass("active");
		else layout.removeClass("active");
		return false;
	});
	go("#layout, #menu, #main").bind("click", function(){
		active = false;
		if (layout.contClass("active")[0]) {
			layout.removeClass("active");
			return false;
		}
	});
}.bind(window)
);

window.addEventListener("popstate", function(e){
	if (e = e.state && e.state.hist) {
		rdr.lockhist = true;
		//var data = {}; data[e] = null;
		ws.send_post(e, {});
	}
});
