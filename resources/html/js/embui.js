var go = function(param, context){
  if (!arguments.length) return new GO(document);
  if (typeof param == "string") return (new GO(context || document)).go(param);
  if (typeof param == "undefined") return new GO();
  return new GO(param);
}

// process form data before serializing and sending to backend
go.formdata = function(form){
  var controls = {},
  chkNumeric = function(element){
    return isFinite(element.value) ? Number(element.value) : element.value;
  },
  checkValue = function(element){
    switch (element.type.toLowerCase()){
      case 'password':
        return element.value;    // keep password field value as-is string
      case 'checkbox':        // use boolean true/false as a value for 'checked' boxes  
        return element.checked;
      case 'number':
      case 'select-one':
      case 'range':
      case 'hidden':
        return chkNumeric(element);
      case 'radio':
        if(element.checked) return chkNumeric(element);
      case 'input':
      case 'textarea':
        return (typeof element.value == 'string' && element.value == "") ? null : this.value;
      default:
        return element.value;
    }
  };

  for(let i = 0; i < form.length; i++){
    let el = form[i];
    if (el.disabled) continue;
    switch (el.tagName.toLowerCase()){
      case 'input':
        var val = checkValue(el);
        if (typeof val != "undefined") controls[el.name || el.id] = val;
        break;
      case 'select':
        controls[el.name || el.id]= chkNumeric(el);
        break;
      //case 'textarea':
      default:
        continue;
    }
  }
  return controls;
}

go.linker = function(fn,th){
  var arg = Array.prototype.slice.call(arguments,2);
  return function(){ return fn.apply(th, Array.prototype.slice.call(arguments).concat(arg)); };
}
go.listevent = function(el, evn, fn, opt){
  var self = ((opt instanceof Object)? opt.self : 0) || this;
  el = (el instanceof Array)? el : [el];
  evn = (evn instanceof Array)? evn : [evn];
  var rfn = function(arg){
    return function(e){
      e = e || event;
      var a = arg.slice(); a.unshift(e);
      var retval = fn.apply(self, a);
      if (typeof retval !== 'undefined' && !retval){
        if(e.preventDefault) e.preventDefault();
        else e.returnValue = false;

        if(e.stopPropagation) e.stopPropagation();
        else e.cancelBubble = true;
      }
      return retval;
    };
  }([].slice.call(arguments,4));

  for(var i = 0; i < el.length; i++) for(var j = 0; j < evn.length; j++){
    if(window.addEventListener){
      el[i].addEventListener(evn[j], rfn, opt);
    }else{
      el[i].attachEvent("on"+evn[j], rfn);
    }
  }
};
go.path = function(el, p, st){
  p = (p instanceof Array)? p : p.split(".");
  var i = p.shift();
  if (!i && st && st._parent) return this.path(st._parent._this, p, st._parent);
  if (i == "_this" || i == "_this2" || i == "_this3") {
    return p.length? this.path(st._this, p, st) : el;
  }
  if (i == "_index") return st._index;
  if (i == "_key") return st._key;
  if (i == "_val") return st._val;

  if (!el) return el;
  if (el[i] instanceof Object && p.length) return this.path(el[i], p, st);
  return el[i];
}
// merge data coming in sepparate frames
go.merge = function(base, data, idx) {
  for (var i in data) {
    var id = idx? idx + parseInt(i) : i;
    if (typeof data[i] == 'object') {
      if (typeof base[id] != 'object') base[id] = (data[i] instanceof Array)? [] : {};
      this.merge(base[id], data[i], (i == "block") ? data.idx : undefined);
    } else base[id] = data[i];
  }
  //console.log("merged base", JSON.parse( JSON.stringify(base) ))
  return base;
}
go.eval = function(th, data) {
  return (function(){ return eval(data); }).call(th);
}

var GO = function(context){
  if (context) {
    context = ((context instanceof Object || typeof context == 'object') &&
      typeof context.length !== 'undefined' && context.nodeType == undefined
    )? context : [context];
    for(var i = 0; i < context.length; i++) this.push(context[i]);
  }
}
GO.prototype = new Array();
;(function($_){
  $_._alias = {},
  $_.alias = function(alias){
    for(var a in alias){
      $_[a] = function(path){
        path = (typeof path == 'string')? path.split(".") : path;
        return function(){
          var m = null, out = new GO([]);
          for (var i = 0; i < this.length; i++) {
            if (typeof path == 'function') {
              m = path.apply(this[i], arguments);
            } else {
              var obj = this[i], len = path.length - 1;
              for (var j = 0; obj && j < len; j++) obj = obj[path[j]];
              if (obj && typeof obj[path[len]] !== 'undefined') {
                if (typeof obj[path[len]] == 'function' || typeof obj[path[len]].toString == 'undefined'){
                  m = obj[path[len]].apply(obj, arguments);
                } else if (typeof obj[path[len]] == 'object'){
                  if (arguments.length == 2) obj[path[len]][arguments[0]] = arguments[1];
                  m = this[i];//obj[path[len]];
                } else {
                  if (arguments.length == 1) obj[path[len]] = arguments[0];
                  m = this[i];//obj[path[len]];
                }
              }
            }
            if (m === null) continue;
            if (m === undefined) m = this[i];
            if (!m || typeof m === 'string' || typeof m === 'function' || typeof m.length === 'undefined') {
              m = [m];
            }
            for (var n = 0; n < m.length; n++) out.push(m[n]);
          }
          return out;
        };
      }(alias[a]);
      $_._alias[a] = alias[a];
    }
    return this;
  }
  $_.alias({
    'go':'querySelectorAll',
    'query':'querySelectorAll',
    'attr':'getAttribute','sattr':'setAttribute','rattr':'removeAttribute',
    'byid':'getElementById',
    'bytag':'getElementsByTagName',
    'byname':'getElementsByName',
    'byclass':'getElementsByClassName',
    'css':'style',
    'style':'style',
    'display':'style.display',
    'left': 'style.left', 'right': 'style.right', 'top': 'style.top', 'bottom': 'style.bottom',
    'height': 'style.height', 'width': 'style.width',
    'innerHTML': 'innerHTML', 'src': 'src', 'href': 'href',
    'addClass':'classList.add', 'removeClass':'classList.remove', 'className':'className',
    'toggleClass':'classList.toggle', 'contClass':'classList.contains',
    'each': function(fn){
      var r = fn.apply(this, [].slice.call(arguments,1));
      return (typeof r != 'undefined')? r : this;
    },
    'exec': function(){ return this.apply(arguments); },
    'event': function(n,dt,ps) {
      var evt = null;
      ps = ps || {bubbles: false, cancelable: true, detail: undefined, view: undefined};
      ps.detail = dt;
      if (typeof window.CustomEvent != "function") {
        evt = document.createEvent('CustomEvent');
        evt.initCustomEvent(n, ps.bubbles, ps.cancelable, ps.detail);
      } else {
        evt = new CustomEvent(n, ps);
      }
      this.dispatchEvent(evt);
      return this;
    },
    'bind': function(ev, fn){
      return go.listevent.apply(this, [this, ev, fn, {self: this}].concat([].slice.call(arguments, 2)));
    },
    'find': function(k,v){
      return (typeof this == 'object' && this[k] && (!v || this[k] === v))? this : null;
    },
    'id': function(v){
      return (typeof this == 'object' && this["id"] === v)? this : null;
    },
    'set': function(k,v){
      if (typeof this == 'object') {
        if (typeof k == 'object') {
          for (var i in k) this[i] = k[i];
        } else this[k] = v;
        return this;
      }
      return null;
    },
    'cstyle': function(ps){
      if (typeof ps == 'object') for (var i in ps) {
        var st = i.split("-");
        for (var n = 1; n < st.length; n++) st[n] = st[n][0].toUpperCase() + st[n].slice(1);
        this.style[st.join("")] = ps[i];
      }
      return [window.getComputedStyle? window.getComputedStyle(this, (typeof ps == 'string')? ps : "") : this.currentStyle];
    },
    'showhide': function(){
      if (!this.style.display) this.style.display = go(this).cstyle()[0].display;
      this.style.display = (this.style.display === "none")? "block" : "none";
      return this;
    },
    'clone': function(){ return this.cloneNode(true); },
    'fragment': function(el){ return document.createDocumentFragment();  },
    'remove': function(){ var p = this.parentNode; p.removeChild(this); return this; },
    'clear': function(){ while(this.firstChild) this.removeChild(this.firstChild); return this; },
    'append': function(el){
      el = (el instanceof Array)? el : [el];
      for(var i = 0; i < el.length; i++) this.appendChild(el[i]);
      return this;
    },
    'prepend': function(el){
      el = (el instanceof Array)? el : [el];
      for(var i =  el.length; i > 0; i--) this.insertBefore(el[i-1], this.firstChild);
      return this;
    },
    'replace': function(el){
      el = (el instanceof Array)? el : [el];
      for(var i = 0; i < el.length; i++) {
        this.parentNode.insertBefore(el[i], this);
        if (i == el.length - 1) this.parentNode.removeChild(this);
      }
      return this;
    },
    'before': function(el){
      el = (el instanceof Array)? el : [el];
      for(var i =  el.length; i > 0; i--) this.parentNode.insertBefore(el[i-1], this);
      return this;
    },
    'after': function(el, t){
      el = (el instanceof Array)? el : [el];
      var ns = t? this.nextSibling : this.nextElementSibling;
      if (ns) {
        for(var i = el.length; i > 0; i--) this.parentNode.insertBefore(el[i-1], ns);
      } else {
        for(var i = 0; i < el.length; i++) this.parentNode.appendChild(el[i]);
      }
      return this;
    },
    'create': function(tg,a1,a2){
      tg = document.createElement(tg);
      var dt = (typeof a2 == "string")? a2 : ((typeof a1 == "string")? a1 : 0),
      at = (typeof a1 == "object")? a1 : 0;
      if(dt) tg.innerHTML = dt;
      if(at) for(var i in at) tg.setAttribute(i, at[i]);
      if (this !== document) this.appendChild(tg);
      return tg;
    },
    'html': function(vl,ap){ if(ap) this.innerHTML += vl; else this.innerHTML = vl; return this; },
    'children': function(){ return this.children; },
    'nodes': function(type){
      return (this.nodeType == (type? type : 1))? this : null;
    }
  });
}(GO.prototype));

var mustache = function(tmpl,func){
  this.nevent = 0; this.scope = [];
  this._tmpl = (typeof tmpl == 'string')? tmpl : tmpl.innerHTML.replace(/[\r\t\n]/g, "");
  this._func = func || this._func || {};

  this._func.exec = this._func.exec || function(d,fn){
    var arg = [].slice.call(arguments, 2),
    out = 'data-musfun'+this.nevent+'="'+fn+'" '+
    'data-musarg'+this.nevent+'="'+escape(JSON.stringify(arg))+'"';
    this.nevent++;
    return out;
  };
  this._func.onevent = this._func.onevent || function(d){
    var arg = [].slice.call(arguments, 1);
    arg.unshift(d, "event");
    return this._func.exec.apply(this, arg);
  };
  this._func.event = this._func.event || function(th,ev,fn){
    var arg = [].slice.call(arguments, 3);
    fn = th._func[fn];
    if (fn) go.listevent(this, ev.split(","), function(e){
      var a = arg.slice(); a.unshift(e); return fn.apply(this, a);
    }, {self: this});
  };
  this._func.if = this._func.if || function(d){
    for (var i = 1; i < arguments.length; i++) if (!arguments[i]) return 0;
    return 1;
  };
  this._func.if2 = this._func.if,
  this._func.if3 = this._func.if,
  this._func.calc = this._func.calc || function(d){
    return [].slice.call(arguments, 1).join();
  };
}
;(function($_){
  var cont = document.createElement("div"),
  rgobj = /\{\{(@|#|!|\^)([a-z0-9_.]+?)(?:\s+(.+?))?\}\}([\s\S]*?)\{\{\/\2\}\}/gi,
  rgitem = /\{\{(!|>)?([a-z0-9_.]+?)(?:\s+(.+?))?\}\}/gi,
  rgparam = /\s*,\s*/gi,
  rgfield = /[a-z0-9_.]+|".*?"|'.*?'|`.*?`/gi,
  strcon = ['true','false','null','undefined','typeof','instanceof','Array','Object'];

  $_._parse_param = function(val, data, stack){
    var m = val.match(rgfield), s = val.split(rgfield), out = "";
    while (s && (val = s.shift()) != undefined) {
      out += val;
      if (m && (val = m.shift())) {
        if (strcon.indexOf(val) != -1 || val[0] == '"' || val[0] == "'" || Number(val) == Number(val)) out += val;
        else if (val[0] == "`") out += val.substr(1, val.length - 2);
        else {
          this.scope.push(go.path(data, val, stack));
          out += "this["+(this.scope.length-1)+"]";
        }
      }
    }
    try{ return go.eval(this.scope, out); }catch(e){ return undefined; }
  }
  $_._parse_item = function(tmpl, data, stack){
    tmpl = (tmpl || "").replace(rgitem, go.linker(function(all, pref, name, argum){
      var arg = argum? argum.split(rgparam) : [], out;
      for (var i = 0; i < arg.length; i++) arg[i] = this._parse_param(arg[i], data, stack);
      if (pref == "!") return "";
      if (pref == ">"){
        var el = document.getElementById(name), dt = arg.length? arg[0] : data;
        return el? (new mustache(el, this._func)).parse(dt, this, stack) : "";
      }
      if (this._func[name]) {
        arg.unshift(data);
        return this._func[name].apply(this, arg);
      }
      out = go.path(data, name, stack);
      // treat undef/null values as empty string ""
      if (typeof out == 'undefined' || typeof out == 'object')
        return "";

      return out;
    }, this));
    return tmpl;
  }
  $_._parse_obj = function(tmpl, data, stack){
    tmpl = (tmpl || "").replace(rgobj, go.linker(function(all, pref, name, argum, body){
      var out = body || argum || "", value = go.path(data, name, stack);
      if (pref == "!") return "";
      if (typeof value == 'function' || this._func[name]) {
        var arg = (body && argum)? argum.split(rgparam) : [];
        for (var i = 0; i < arg.length; i++) arg[i] = this._parse_param(arg[i], data, stack);
        arg.unshift(data);
        if (pref == "@") arg.unshift(body);
        var res = (this._func[name] || value).apply(this, arg);
        if (pref == "#") out = res? this._parse_obj(out, data, stack) : "";
        else if (pref == "^") out = res? "" : this._parse_obj(out, data, stack);
        else out = res;
      } else if (value instanceof Array) {
        if (pref != "^" && !value.length) return "";
        if (pref == "^" && value.length) return "";
        var res = "", stk = {_this:data, _parent:stack};
        for (var i = 0; i < value.length; i++) {
          stk._index = i; stk._val = value[i];
          var tmp = this._parse_obj(out, value[i], stk);
          res += this._parse_item(tmp, value[i], stk);
        }
        out = res;
      } else if (value instanceof Object) {
        if (!value && pref == "#" || value && pref == "^") return "";
        var stk = {_this:data, _parent:stack};
        if (pref == "#" || pref == "^") {
          out = this._parse_obj(out, value, stk);
          out = this._parse_item(out, value, stk);
        } else {
          var res = "", n = 0;
          for (var i in value) {
            stk._index = n++; stk._key = i;
            var tmp = this._parse_obj(out, value[i], stk);
            res += this._parse_item(tmp, value[i], stk);
          }
          out = res;
        }
      } else {
        if (!value && pref == "#" || value && pref == "^") return "";
        out = this._parse_obj(out, data, stack);
        out = this._parse_item(out, data, stack);
      }

      return out;
    }, this));
    return tmpl;
  }
  $_.parse = function(data, parent, stack){
    this.nevent = (typeof parent == "object")? parent.nevent : 0;
    stack = stack || {_this:data};
    var out = this._parse_obj(this._tmpl, data, stack);
    out = this._parse_item(out, data, stack);
    if (parent) {
      if (typeof parent == "object") parent.nevent = this.nevent;
      return out;
    }
    cont.innerHTML = out;
    for (var i = 0; i <= this.nevent; i++){
      go("[data-musfun"+i+"]",cont).each(function(th){
        var fn = th._func[this.getAttribute("data-musfun"+i)],
        arg = JSON.parse(unescape(this.getAttribute("data-musarg"+i)));
        this.removeAttribute("data-musarg"+i);
        this.removeAttribute("data-musfun"+i);
        try{ if (fn){ arg.unshift(th); fn.apply(this, arg); } }catch(e){}
      }, this);
    }
    return [].slice.call(cont.childNodes);
  }
}(mustache.prototype));

var wbs = function(url){
  var ws = null, frame = {}, connected = false, lastmsg = null, to = null,
  open = function(fnopen, fnerror){
    ws = new WebSocket(url);
    ws.onerror = function(err){
      console.log("WS Error", err);
      if (fnerror) fnerror(err);
      if (typeof out.onerror== "function") {
        try{ out.onerror(); } catch(e){ console.log('Error onerror', e); }
      }
    }
    ws.onopen = function(){
      console.log("WS Open");
      if (fnopen) fnopen();
      if (typeof out.onopen== "function") {
        try{ out.onopen(); } catch(e){ console.log('Error onopen', e); }
      }
    },
    ws.onclose = function(){
      console.log("WS Close");
      if (typeof out.onclose== "function") {
        try{ out.onclose(); } catch(e){ console.log('Error onclose', e); }
      }
    }
    ws.onmessage = function(msg){
      let m = {};
      try{ m = JSON.parse(msg.data); } catch(e){ console.log('Error message', e); return; }
      //console.log('Received message:', msg.data);
      if (!(m instanceof Object)) return;
      if (m.section && !(m.section in frame) && m.final){
        receiv_msg(m);
        return;
      }
      if (m.section) {
        if (!frame[m.section]) frame[m.section] = {};
        go.merge(frame[m.section], m);
        if (m.final) {
          receiv_msg(frame[m.section]);
          delete frame[m.section];
        }
      } else {
        receiv_msg(m);
      }
    }
  },
  receiv_msg = function(msg){
    console.log('Received packet:', JSON.parse(JSON.stringify(msg)));
    if (msg.pkg && typeof out["on"+msg.pkg] == "function"){
      try{ out["on"+msg.pkg](msg); } catch(e){ console.log('Error on'+msg.pkg, e); }
    } else {
      try{ out["onUnknown"](msg); } catch(e){ console.log('Error on'+msg.pkg, e); }
    }
  },
  send = function(msg){ try{ ws.send(msg); }catch(e){} },
  send_msg = function(msg){
    console.log('Sending message:', msg);
    try{ lastmsg = JSON.stringify(msg); } catch(e){ console.log('Error stringify', e); return; }
    if (ws.readyState === WebSocket.OPEN){
      send(lastmsg);
      lastmsg = null;
    }
  },

  out = {
    connect: function(){
      console.log("WS Connect");
      ws = null;
      if (to) clearTimeout(to);
      to = setTimeout(function(){open(function(){
        to = null;
        if (lastmsg){ send(lastmsg); lastmsg = null; }
      })}, 500);
    },
    // send an action with optional data object
    send_post: function(id, val){
      //console.log("POST action:", id, "data:", val);
      send_msg({pkg:"post", "action":id, "data":val});
    }
  };
  return out;
}

// ==============================
// DyncCSS
function changeCSS(cssFile, cssLinkIndex) {

    var oldlink = document.getElementsByTagName("link").item(cssLinkIndex);

    var newlink = document.createElement("link");
    newlink.setAttribute("rel", "stylesheet");
    newlink.setAttribute("type", "text/css");
    newlink.setAttribute("href", cssFile);

    document.getElementsByTagName("head").item(0).replaceChild(newlink, oldlink);
}

function setDynCSS(url) {
        if (!arguments.length) {
                url = (url = document.cookie.match(/\bdyncss=([^;]*)/)) && url[1];
                if (!url) return '';
        }
        document.getElementById('dyncss').href = url;
        var d = new Date();
        d.setFullYear(d.getFullYear() + 1);
        document.cookie = ['dyncss=', url, ';expires=', d.toGMTString(), ';path=/;'].join('');
        return url;
}

setDynCSS();


// ==============================
// MAKER code

/**
 * global objects placeholder
 */
var global = {
  menu_id:0,
  menu: [],
  value:{},
  manifest:{
	app:"embui",
	appver:0,
	appjsapi:0,
	macid:"000000",
	uiver:0,
	uijsapi:0,
	uiobjects:0,
    lang:"en"
  }
};

/**
 * EmbUI's js api version
 * used to set compatibilty dependency between backend firmware and WebUI js
 */
const ui_jsapi = 9;

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
 * Data could be sent from the controller with any 'pkg' types except internal ones used by EmbUI
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
 *   'value' is passed value
 */
var customFuncs = {
  // BasicUI class - set system date/time from the browser
  set_time: function (event) {
    var t = new Date();
    var tLocal = new Date(t - t.getTimezoneOffset() * 60 * 1000);
    var isodate = tLocal.toISOString().slice(0, 19);  // chop off chars after ss

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
      ws.send_post("sys_datetime", isodate);
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
 *   "pkg":"xload" messages are used to make ajax requests for external JSON objects that could be used as data/interface templates
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
      var rdr = this.rdr = render();  // Interface rederer to pass updated objects to
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
              delete element.url;  // удаляем url из элемента т.к. работает рекурсия
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
function recurseforeachkey(obj, f, ...args) {
  for (let key in obj) {
    if (typeof obj[key] === 'object') {
      if (Array.isArray(obj[key])) {
        for (let i = 0; i < obj[key].length; i++) {
          recurseforeachkey(obj[key][i], f, args);
        }
      } else {
        recurseforeachkey(obj[key], f, args);
      }
    } else {
      // run callback
      f(key, obj, args);
    }
  }
}

// recursive block find, look for an obj with matching key,val
function findBlockElement(array, key, value) {
  let o;
  array.some(function iter(a){
      if (a[key] === value) {
          o = a;
          return true;
      }
      return Array.isArray(a.block) && a.block.some(iter);
  });
  return o;
}

/**
 * scan frame object for 'uidata' instruction objects
 * loads/updates objects in uidata container or implaces data into packet from uidata
 * 
 * @param {*} arr a reference to  section.block array
 * @returns 
 */
async function process_uidata(arr){
  if (!(arr instanceof Array))return

  for (let i = 0; i != arr.length; ++i){
    let item = arr[i]
    // skip objects without 'block' key
    if (!(item.block instanceof Array)){
      continue
    } 

    // process uidata sections
    if (item.section == "uidata" ){
      let newblocks = []  // an array for sideloaded blocks
      // scan command objects
      for await (aw of item.block){
        if(aw.action == "xload"){
          // obj can mutate hell knows why while promise it resolved, so make a deep copy here
          let lobj = structuredClone(aw)
          const req = await fetch(lobj.url, {method: 'GET'});
          if (!req.ok){
            console.log("Xload failed:", req.status);
            return;
          } 
          const response = await req.json();
          //console.log("Get responce for:", lobj.url);
          if (lobj.merge){
            if (lobj.src)
              _.merge(_.get(uiblocks, lobj.key), _.get(response, lobj.src))
            else
              _.merge(_.get(uiblocks, lobj.key), response)
            //console.log("Merged key:", lobj.key, lobj.src, aw.key, aw.src);
          } else {
            if (lobj.src)
              _.set(uiblocks, lobj.key, _get(response, lobj.src))
            else
            _.set(uiblocks, lobj.key, response);
            //console.log("Set key:", lobj.key, lobj.src, obj.key, obj.src);
          }
          //console.log("loaded uiobj:", _.get(uiblocks, obj.key));
          //console.log("loaded uiobj:", response, " ver:", response.version);
          // check if loaded data is older then requested from backend
          //console.log("Req ver: ", v.version, " vs loaded ver:", _.get(uiblocks, v.key).version)
          if (lobj.version > _.get(uiblocks, lobj.key).version){
            console.log("Opening update alert msg");
            document.getElementById("update_alert").style.display = "block";
          }
          continue
        }
        // Pick UI object from a previously loaded UI data storage
        if (aw.action == "pick"){
          //console.log("pick obj:", aw.key, _.get(uiblocks, aw.key));
          let ui_obj = structuredClone(_.get(uiblocks, aw.key))  // make a deep-copy to prevent mangling with further processing
          if (ui_obj == undefined){
            // alternate object is available?
            if (aw.alt){
              //console.log("UIData alt:", aw.key, aw.alt);
              ui_obj = aw.alt
            }
            else {
              console.log("UIData is missing:", aw.key);
              continue
            }
          }
          let add_prefix_suffix = function(pick_obj, uidata_obj){
            if (Object.keys(uidata_obj).length !== 0){
              if (pick_obj.prefix){
                uidata_obj.section = pick_obj.prefix + uidata_obj.section;
                recurseforeachkey(uidata_obj, function(key, object){ if (key === "id"){ object[key] = aw.prefix + obj[key]; } })
              }
              if (pick_obj.suffix){
                uidata_obj.section += pick_obj.suffix;
                recurseforeachkey(uidata_obj, function(key, object){ if (key === "id"){ object[key] += aw.suffix; } })
              }
              if (pick_obj.newid){
                uidata_obj["id"] = pick_obj.newid
              }
            }
          }
          // we have an array in ui data, iterate it
          if ((ui_obj instanceof Array)){
            for (el of ui_obj){
              add_prefix_suffix(aw, el);
              newblocks.push(el)
            }
            continue
          }
          // have a single obj, change pref/siff if needed
          if ((ui_obj instanceof Object)){
            add_prefix_suffix(aw, ui_obj);
            newblocks.push(ui_obj)
          }
          // go on with next ui item in this section
          continue
        }
        // Set/update UI object with supplied data
        if (obj.action == "set"){
          _.set(uiblocks, obj.key, obj.data)
          continue
        }
        // Set/update UI object with supplied data
        if (obj.action == "merge"){
          _.merge(_.get(uiblocks, obj.key), obj.data)
          continue
        }
  
      } // (aw of item.block)

      // a function that will replace current section item with uidata items
      function implaceArrayAt(array, idx, arrayToInsert) {
        Array.prototype.splice.apply(array, [idx, 1].concat(arrayToInsert));
        return
      }

      // replace processed section with processed data
      if (newblocks.length){
        implaceArrayAt(arr, i, newblocks)
      } else
        arr.splice(i, 1)  // remove uidata section

        // since array has changed, I must reiterate it all over again
        const chained = await process_uidata(arr)
        // after reiteration no need to continue
      return
    }

    // for non-uidata objects just dive into nested block sections
    if (item.block instanceof Array){
      await process_uidata(item.block)
    }
  }
}


/**
 * recourse object looking for section named "xload",
 * each object inside this section is checked for {"xload":true} pair, if found then an external object is http-loaded via "xload_url" URL and merged into block:[] array
 * @note on load, "xload" key is set to result state of an http load
 * @note on load, the section's value is renamed to the value of "sect_id" key, if "sect_id" is not present, then "xload_{somerandomstring}" is used
 * @param {*} obj - receives an objects referencing section in an "pkg":"interface" frame, or the frame object itself
 */
async function process_XLoadSection(arr){
  if (!(arr instanceof Array)) return
  for await (item of arr){
    // skip objects without 'block' obj
    if (!(item.block instanceof Array)) continue;

    // process xload sections
    if (item.section == "xload" ){
      for await (obj of item.block){
        if (obj.xload && obj.xload_url){
          const req = await fetch(obj.xload_url, {method: 'GET'})
          if (req.ok){
            const resp = await req.json();
            if (resp instanceof Array)
              obj.block.push(...resp)
            else
              obj.block.push(resp)

            obj.xload = true
          } else
            obj.xload = false

          if (item.sect_id){
            item.section = item.sect_id
            delete item.sect_id
          } else
            item.section += Math.round( Math.random() * 1000 )
        }
      }
      continue
    }
    // for non-xload section just dive into nested block sections
    if (item.block instanceof Array)
      await process_XLoadSection(item.block)
  }
  return
}

function extend_ids(obj, prefix){
  if (!(obj instanceof Object) || !(obj.block instanceof Array)) return
  if (obj.extended_ids)
    prefix += prefix ? '.' + obj.section : obj.section

  for (item of obj.block){
    if (item instanceof Object && ("section" in item)){
      extend_ids(item, prefix)
      continue
    }
    if (obj.extended_ids && ("id" in item) )
      item.id = prefix + '.' + item.id
  }
}

// Page data renderer
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
/*
      if (this.id != id){
        custom_hook(this.id, d, id);
      }
*/
    },
    // handle dynamicaly changed elements on a page which should send value to WS server on each change
    on_change: function(d, id, val) {
      console.log("execute on_change, id:", id, ", val:", val);
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
/*
      if (this.id != id){
        custom_hook(this.id, d, id);
      }
*/
      ws.send_post(id, value);
    },
    // show or hide section on a page
    on_showhide: function(d, id) {
      go("#"+id).showhide();
    },
    /**
     *  Process Submited form
     *  'submit' event defines action to be taken by the backend
     */
    on_submit: function(d, id, val, extended_ids) {
      //console.log("execute on_submit, id:", id, ", val:", val);
      let form = go("#"+id), data = go.formdata(go("input, textarea, select", form));
      // submit executor _might_ provide it's own value
      if (val !== undefined && typeof val !== 'object'){
        // if data is an empty object (i.e. no form data, reassign a supplied value to it)
        if (Object.keys(obj).length)
          data[id] = val;
        else
          data = val;
      }
      if (extended_ids){
        let d = {}
        for (const [key, v] of Object.entries(data)){
          _.set(d, key, v)
        }
        ws.send_post(id, d[id]);
      } else
        ws.send_post(id, data);
    },
    // run custom user-js function
    on_js: function(event, callback, arg, id) {
      if (callback in customFuncs){
        //console.log("run user function:", callback, ", arg:", arg, ", id:", id);
        customFuncs[callback](event, arg, id);
      } else
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
      ws.send_post(menu_id);
    },
    menu: function(){
      go("#menu").clear().append(tmpl_menu.parse(global));
    },
    content: function(obj){
      //console.log("Process content:", obj);
      for (i of obj.block){
        go("#"+i.id).replace(tmpl_content.parse(i));
      }
    },
    section: function(obj){
      if (obj.main) {
        go("#main > section").remove();
        go("#main").append(tmpl_section_main.parse(obj));
        if (!out.lockhist) out.history(obj.section);
      } else {
        if (obj.replace){
          console.log("replacing section:", obj.section)
          go("#"+obj.section).replace(tmpl_section.parse(obj));
        } else if (obj.append){
          console.log("Append to section:", obj.section)
          go("#"+obj.append).append(tmpl_section.parse(obj));
        } else {
          // ( Object.keys(go("#"+obj.section)).length === 0 && !obj.replace )
          go("#main").append(tmpl_section_main.parse(obj));
        }
      }
    },
    // process "pkg":"interface" messages and render template
    make: async function(obj){
      if (!obj.block) return;
      let frame = obj.block;

      // deep iterate and process "uidata" sections
      const ui_processed = await process_uidata(frame);
      const xload_processed = await process_XLoadSection(frame);
      extend_ids(obj, "")
      console.log("Processed packet:", obj);

      // go through sections and render it according to type of data
      frame.forEach(function(v, idx, frame){
        if (v.section == "manifest"){
          for (item of v.block){
            _.merge(global, "manifest", item)
          }
          if (global.manifest.app && global.manifest.mc)
            document.title = global.manifest.app + " - " + global.manifest.mc;

    		  if (global.manifest.uijsapi > ui_jsapi || global.manifest.appjsapi > app_jsapi || (uiblocks.sys.version != undefined && global.manifest.uiobjects > uiblocks.sys.version))
            document.getElementById("update_alert").style.display = "block";
          return;
        }

        if (v.section == "menu"){
          global.menu =  v.block;
          if (!global.menu_id) global.menu_id = global.menu[0].value
          this.menu();
          return;
        }

        if (v.section == "content") {
          this.content(v)
          return;
        }

        // callback section contains actions that UI should request back from MCU
        if (v.section == "callback") {
          for (item of v.block){
            let data = undefined;
            if (item.data)
              data = item.data
            console.log("sending callback - action:", item.action, "data:", data)
            ws.send_post(item.action, data);
          }
          return;
        }

        // section with value - process as value frame
        if (v.section == "value") {
          this.value(v)
          return;
        }

        // look for nested xload sections and await for side-load, if found
        //section_xload(v).then( () => { this.section(v); } );
        this.section(v)
      }, this)  // need this to refer to .menu and .section inside forEach
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
        if (val == null || typeof val == "object") return;  // skip undef/null or (empty) objects
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

        if (el[0].type == "range") { go("#"+el[0].id+"-val").html(": "+val); }    // update span with range's label

        //console.log("Element is: ", el[0], " class is: ", el[0].className);
        el[0].value = val;
      }

      for (var i = 0; i < frame.length; i++) if (typeof frame[i] == "object") {
        /* check if the object contains just an object with key:value pairs (comes from echo-back packets)
          { "id": "someid", "value": "somevalue", "html": true }
        */
        if ('id' in frame[i] && 'value' in frame[i]){
          setValue(frame[i].id, frame[i].value, frame[i].html);
        } else {
          // else it's an object with k:v pairs
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

  // process "pkg":"interface"
  ws.oninterface = function(msg) { rdr.make(msg) }

  // run custom js function in window context
  ws.onjscall = function(msg) {
    if ( msg.jsfunc in customFuncs){
      try {   console.log("JSCall: ", msg.jsfunc);
        customFuncs[msg.jsfunc](msg)
        //window[msg.jsfunc](msg);
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
  let response = await fetch("/js/ui_embui.json", {method: 'GET'});
  if (response.ok){
    response = await response.json();
    uiblocks['sys'] = response;
    //console.log("loaded obj:", uiblocks.sys);
  }
  // preload i18n
  //let i18n = await fetch("/js/ui_embui.i18n.json", {method: 'GET'});
  //let lang = await fetch("/js/ui_embui.lang.json", {method: 'GET'});

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
    ws.send_post(e);
  }
});
