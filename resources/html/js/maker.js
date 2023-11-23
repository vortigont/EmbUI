
/**
 * global objects placeholder
 */
var global = {menu_id:0, menu: [], value:{}};


/**
 * EmbUI's js api version
 * used to set compatibilty dependency between backend firmware and WebUI js
 */
const ui_jsapi = 3;

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
	dtime: function (v) {
		var t = new Date();
		var tLocal = new Date(t - t.getTimezoneOffset() * 60 * 1000);
		var isodate = tLocal.toISOString();
		isodate = isodate.slice(0, 19);	// chop off chars after ss

		var data = {};
		if (typeof v == 'undefined' || typeof v == 'object'){
			// if there was no 'value' given, than simply post the date string to MCU that triggers time/date setup
			data["set_sys_datetime"] = isodate;
			ws.send_post("set_sys_datetime", data);
		} else {
			// if there was a param 'value', then paste the date string into doc element with id='value'
			// let's do this via simulating MCU value frame
			data["block"] = [];
			data.block.push({"date" : isodate});
			var r = render();
			r.value(data);
		}
	}//,
	//func2: function () {     console.log('Called func 2'); }
};

/**
 * rawdata callback - Stub function to handle rawdata messages from controller
 * Data could be sent from the controller via json_frame_custom(String("rawdata")) method
 * and handled in a custom user js script via redefined function
 */
function rawdata_cb(msg){
    console.log('Got raw data, redefine rawdata_cb(msg) func to handle it.', msg);
}

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

// template renderer
var render = function(){
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
		// handle dynamicaly changed elements on a page
		on_change: function(d, id, val) {

			chkNumeric = function(v){
				// cast empty strings to null
				if (typeof v == 'string' && v == "") return null;
				if(isFinite(v))
					return Number(v);
				else
					return v;
			};

			let value;

			// check if value has been supplied by templater
			if (val !== undefined){
				value = val;
			} else {
				switch (this.type){
					case 'checkbox':
						value = document.getElementById(id).checked;
						break;
					case 'input':
					case 'select-one':
					case 'range':
						value = chkNumeric(this.value);
						break;
					case 'textarea':	// cast empty strings to null
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
		on_js: function(d, id, val) {
			if (id in customFuncs)
				customFuncs[id](val);
			else
				console.log("Custom func undefined: ", id);
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
				go("#"+obj.section).replace(tmpl_section.parse(obj));
			}
		},
		// process "pkg":"interface" messages and render template
		make: function(obj){
			if (!obj.block) return;
			let frame = obj.block;
			for (let i = 0; i < frame.length; i++) if (typeof frame[i] == "object") {
				if (frame[i].section == "uidata"){
					// process section with uidata objects
					let newblock = []	// an array for sideloaded blocks
					frame[i].block.forEach(function(v, idx, arr){
						if (v.action == "pick"){
							newblock.push(_.get(uiblocks, v.key))
							return
						}
					})
					// a function that will replace current section item with uidata items
					function replaceArrayAt(array, idx, arrayToInsert) {
						Array.prototype.splice.apply(array, [idx, 1].concat(arrayToInsert));
						return array;
					}
					if (newblock.length)
						replaceArrayAt(frame, i, newblock)
					else
						obj.splice(i, 1)

					return this.make(obj)	// since array length has changed, we just restart make() again over changed object	
				}

				// check top-level section type for any predefined ID that must processed in a specific way
				if (frame[i].section == "content") {
					for (let n = 0; n < frame[i].block.length; n++) {
						go("#"+frame[i].block[n].id).replace(tmpl_content.parse(frame[i].block[n]));
					}
					continue;
				}
				if (frame[i].section == "manifest"){
					let manifest = frame[i].block[0];
					document.title = manifest.app + " - " + manifest.mc;
					global.app = manifest.app;
					global.macid = manifest.mc;
					global.uiver = manifest.uiver;
					global.uijsapi = manifest.uijsapi;
					global.appver = manifest.appver;
					global.appjsapi = manifest.appjsapi;
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
				Sets the value 'val' to the DOM object with id 'key'
				if 'html' is set to 'true', than values applied as html-text value,
				i.e. template tags visible on the page  ( <span>{{value}}</span> )
				otherwise value applies as html element attribute ( <input type="range" value="{{value}}" )
			*/
			function setValue(key, val, html = false){
				if (val == null || typeof val == "object") return;	// skip undef/null or (empty) objects
				let el = go("#"+key);
				if (!el.length) return;
				if (html === true ){ el.html(val); return; }

				if (el[0].type == "range") { go("#"+el[0].id+"-val").html(": "+el[0].value); return; }		// update span with range's label
				// проверяем чекбоксы на значение вкл/выкл
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

				//console.log("Element is: ", el[0], " class is: ", el[0].className);
				el[0].value = val;
			}

			for (var i = 0; i < frame.length; i++) if (typeof frame[i] == "object") {

				// check if the object contains just an array with key:value pairs (comes from echo-back packets)
				if (!frame[i].id && !frame[i].value){
					for(var k in frame[i]) {
						setValue(k, frame[i][k]);
					}
					continue;
				}

				/* otherwise it must be a dict
					{ "id": "someid",
					  "value": "somevalue"
					},
				*/
				setValue(frame[i].id, frame[i].value, frame[i].html);
			}
		}
	};
	return out;
};



window.addEventListener("load", function(ev){
	var rdr = this.rdr = render();
	var ws = this.ws = wbs("ws://"+location.host+"/ws");

	ws.oninterface = function(msg) {
		rdr.make(msg);
	}
	ws.onvalue = function(msg){
		rdr.value(msg);
	}
	ws.onclose = ws.onerror = function(){
		ws.connect();
	}

	// any messages with "pkg":"rawdata" are handled here bypassing interface/value handlers
	ws.onrawdata = function(mgs){
		rawdata_cb(mgs);
	}

	// "pkg":"xload" messages are used to make ajax requests for external JSON objects that could be used as data/interface templates
	ws.onxload = function(mgs){
		xload(mgs);
	}

	// load sys UI objects
	ajaxload("/js/ui_sys.json", function(response) {
		uiblocks['sys'] = response;
	});

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
