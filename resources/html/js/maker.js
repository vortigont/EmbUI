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
			data["time"] = isodate;
			ws.send_post(data);
		} else {
			// if there was a param 'value', then paste the date string into doc element with id='value'
			// let's do this via simulating MCU value frame
			data["block"] = [];
			data.block.push({[v] : isodate});
			var r = render();
			r.value(data);
		}
	}//,
	//func2: function () {     console.log('Called func 2'); }
};

var global = {menu_id:0, menu: [], value:{}};

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
		on_change: function(d, id, val) {

			chkNumeric = function(v){
				// cast empty strings to null
				if (typeof v == 'string' && v == "") return null;
				if(isFinite(v))
					return Number(v);
				else
					return v;
			};

			var value;
			var type = this.type;

			// check if value has been supplied by templater
			if (val !== undefined){
				value = val;
			} else {
				switch (type){
					case 'checkbox':
						var chbox=document.getElementById(id);
						if (chbox.checked) value = true;		// send 'checked' state as boolean true/false
						else value = false;
						break;
					case 'input':
					case 'select-one':
					case 'range':
						value = chkNumeric(this.value);
						break;
					case 'textarea':	// cast empty strings to null
						if (typeof this.value == 'string' && this.value == "")
							value = null;
						else
							value = this.value;
						break;
					default:
						value = this.value;
				}
			}
			if (this.id != id){
				custom_hook(this.id, d, id);
			}

			var data = {}; data[id] = (value !== undefined)? value : null;
			ws.send_post(data);
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
			data['submit'] = id;
			if (val !== undefined && typeof val !== 'object') { data[id] = val; }	// submit button _might_ have it's own additional value
			ws.send_post(data);
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
			var data = {}; data[menu_id] = null
			ws.send_post(data);
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
		make: function(obj){
			var frame = obj.block;
			if (!obj.block) return;
			for (var i = 0; i < frame.length; i++) if (typeof frame[i] == "object") {
				if (frame[i].section == "menu") {
					global.menu =  frame[i].block;
					document.title = obj.app + " - " + obj.mc;
					global.app = obj.app;
					global.mc = obj.mc;
					global.ver = obj.ver;
					if (!global.menu_id) global.menu_id = global.menu[0].value
					this.menu();
				} else
				if (frame[i].section == "content") {
					for (var n = 0; n < frame[i].block.length; n++) {
						go("#"+frame[i].block[n].id).replace(tmpl_content.parse(frame[i].block[n]));
					}
				} else {
					this.section(frame[i]);
				}
			}
			out.lockhist = false;
		},
		// обработка данных, полученных в пакете "pkg":"value"
		// блок разбирается на объекты по id и их value применяются к элементам шаблона на html странице
		value: function(obj){
			var frame = obj.block;
			if (!obj.block) return;

			/*
			Sets the value 'val' to the DOM object with id 'key'  
			*/
			function setValue(key, val){
				if (val == null || typeof val == "object") return;	// skip undef/null or (empty) objects
				var el = go("#"+key);
				if (el.length) {
					if (frame[i].html) {	// update placeholders in html template, like {{value.pMem}}
						global.value[key] = val
						el.html(val);
					} else{
						el[0].value = val;
						if (el[0].type == "range") go("#"+el[0].id+"-val").html(": "+el[0].value);
						// проверяем чекбоксы на значение вкл/выкл
						if (el[0].type == "checkbox") {
							// allow multiple types of TRUE value for checkboxes
							el[0].checked = (val == "1" || val == 1 || val == true || val == "true" );
						}
					}
				}	
			}

			for (var i = 0; i < frame.length; i++) if (typeof frame[i] == "object") {

				// check if the object contains just a plain assoc array with key:value pairs
				if (!frame[i].id && !frame[i].value){
					for(var k in frame[i]) {
						setValue(k, frame[i][k]);
					}
					continue;
				}

				/* otherwise it must be
					{ "id": "someid",
					  "value": "somevalue"
					}
				*/
				setValue(frame[i].id, frame[i].value);
			}
		}
	};
	return out;
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
		var section = obj[i];
		if (section.url){
			console.log('Fetching URL"' + section.url);
			await new Promise(next=> {
					ajaxload(section.url,
						function(response) {
							section['block'] = response;
							delete section.url;	// удаляем url из элемента т.к. работает рекурсия
							// пробегаемся рекурсивно по новым/вложенным объектам
							if (section.block && typeof section.block == "object") { 
								deepfetch(section.block).then(() => {
									//console.log("Diving deeper");
									next();
							   })
							} else {
								next();
							}
						},
						function(errstatus) {
							//console.log('Error loading external content');
							next();
						}
					);
			})
		} else if ( section.block && typeof section.block == "object" ){
			await new Promise(next=> {
				deepfetch(section.block).then(() => {
					next();
				})
			})			
		}
	}
}


window.addEventListener("load", function(ev){
	var rdr = this.rdr = render();
	var ws = this.ws = wbs("ws://"+location.host+"/ws");
	//var ws = this.ws = wbs("ws://embuitst/ws");
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
}.bind(window));

window.addEventListener("popstate", function(e){
	if (e = e.state && e.state.hist) {
		rdr.lockhist = true;
		var data = {}; data[e] = null;
		ws.send_post(data);
	}
});
