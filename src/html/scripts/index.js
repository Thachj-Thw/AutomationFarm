function request(method, url, type, callback=null, json=null){
    var xhr = new XMLHttpRequest();
    xhr.open(method, url);
    if(method == "POST"){
        xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
        // console.log(json);
        xhr.send(JSON.stringify(json));
    }else{
        xhr.send();
    }
    xhr.responseType = type;
    xhr.onload = () => {
        if (xhr.readyState == 4 && xhr.status == 200) {
            // console.log(xhr.response);
            if(callback){
                callback(xhr.response);
            }
        } else {
            console.log(`Error: ${xhr.status}`);
            if(callback){
                callback({status: 1});
            }
        }
    };
}

// Dashboard
const temp_C_value = document.querySelector("#temperature .C.value");
const temp_F_value = document.querySelector("#temperature .F.value");
const humidity_value = document.querySelector("#humidity .value");
const soil_value = document.querySelector("#soil-moisture .value");
const pump_state = document.querySelector("#pump .value");
const flash_state = document.querySelector("#flash .value");
// Control
const cb_pump = document.querySelector("#pump-control input");
const cb_flash = document.querySelector("#flash-control input");
const pump_state_control = document.querySelector("#pump-control .value");
const flash_state_control = document.querySelector("#flash-control .value");
// Setup
const input_soil = document.querySelector("#value-pump-off");
const list_timer = [
    document.querySelector("#time-1"),
    document.querySelector("#time-2"),
    document.querySelector("#time-3"),
    document.querySelector("#time-4"),
    document.querySelector("#time-5")
];
const setup_soil_value = document.querySelector("#value-off-setting .value");
const list_timer_value = [
    document.querySelector("#time-value-1"),
    document.querySelector("#time-value-2"),
    document.querySelector("#time-value-3"),
    document.querySelector("#time-value-4"),
    document.querySelector("#time-value-5")
];
// Save button
const save_btn = document.querySelector("#submit");
// Settime
const settime_btn = document.querySelector("#set-time");
const settime_value = document.querySelector("#currenttime");

settime_btn.onclick = () => {
    var hour_minute = settime_value.value.split(":");
    if (hour_minute.length < 2) {
        alert("Chưa nhập thời gian");
        return;
    }
    var dateObj = new Date();
    var day_month_year = dateObj.toLocaleDateString().split("/");
    request("POST", "/api/set_time", "json", (data) => {
        if (data.status == 0){
            alert("Đặt lại giờ thành công");
            settime_value.value = "";
        } else {
            alert("Đặt lại giờ thất bại");
        }
    },
    {
        "year": parseInt(day_month_year[2]),
        "month": parseInt(day_month_year[1]),
        "day": parseInt(day_month_year[0]),
        "hour": parseInt(hour_minute[0]),
        "minute": parseInt(hour_minute[1]),
        "second": 0
    });
};

function set_pump_on(is_on){
    if (is_on){
        pump_state.textContent = "ON";
        pump_state.classList.add("state-on");
        pump_state_control.textContent = "ON";
        pump_state_control.classList.add("state-on");
        cb_pump.checked = true;
    }else{
        pump_state.textContent = "OFF";
        pump_state.classList.remove("state-on");
        pump_state_control.textContent = "OFF";
        pump_state_control.classList.remove("state-on");
        cb_pump.checked = false;
    }
}

function set_flash_on(is_on){
    if (is_on){
        flash_state.textContent = "ON";
        flash_state.classList.add("state-on");
        flash_state_control.textContent = "ON";
        flash_state_control.classList.add("state-on");
        cb_flash.checked = true;
    }else{
        flash_state.textContent = "OFF";
        flash_state.classList.remove("state-on");
        flash_state_control.textContent = "OFF";
        flash_state_control.classList.remove("state-on");
        cb_flash.checked = false;
    }
}

cb_pump.onclick = () => {
    if (cb_pump.checked){
        request("GET", "/api/pump_on", "json", (data) => {
            if (data.status == 0){
                set_pump_on(true);
            } else {
                alert("Bật bơm thất bại");
                cb_pump.checked = false;
            }
        });
    }else{
        request("GET", "/api/pump_off", "json", (data) => {
            if (data.status == 0){
                set_pump_on(false);
            } else {
                alert("Tắt bơm thất bại");
                cb_pump.checked = true;
            }
        });
    }
};

cb_flash.onclick = () => {
    if (cb_flash.checked){
        request("GET", "/api/flash_on", "json", (data) => {
            if (data.status == 0){
                set_flash_on(true);
            } else {
                alert("Bật đèn thất bại");
                cb_flash.checked = false;
            }
        });
    }else{
        request("GET", "/api/flash_off", "json", (data) => {
            if (data.status == 0){
                set_flash_on(false);
            } else {
                alert("Tắt đèn thất bại");
                cb_flash.checked = true;
            }
        });
    }
};

function minuteAsStringTime(m, _default = ""){
    if (m < 0) return _default;
    var h = parseInt(m / 60);
    var m = m % 60;
    return (h < 10 ? "0" + h: h) + ":" + (m < 10 ? "0" + m : m);
}

function getData(){
    request("GET", "/api/get", "json", updateData);
}

function updateData(data){
    //data struct {status: 0, temp: '0', humidity: '0', soil: '0.0', pump: false, flash: false, soil_set: 100.0, timer: [-1, -1, -1, -1, -1]}
    if (data.status == 0){
        temp_C_value.textContent = data.temp;
        temp_F_value.textContent = (parseInt(data.temp) * 1.8 + 32).toFixed(2);
        humidity_value.textContent = data.humidity;
        soil_value.textContent = data.soil.toFixed(2);
        set_pump_on(data.pump);
        set_flash_on(data.flash);
        setup_soil_value.textContent = data.soil_set + "%";
        for (let i = 0; i < 5; i++) {
            list_timer_value[i].textContent = minuteAsStringTime(data.timer[i], "chưa đặt");
        }
    }
    setTimeout(getData, 3000);
}

const camera = document.querySelector("#camera img");

function takeCapture(){
    request("GET", "/api/capture", "arraybuffer", (arrayBuffer) => {
        camera.setAttribute("alt", "Lấy ảnh chụp thất bại. Nhấn để thử lại");
        camera.src = "/api/capture";
    });
}

function stringTimeAsMinute(t){
    if (!t) { return -1 }
    var h_m = t.split(":");
    var h = parseInt(h_m[0]);
    var m = parseInt(h_m[1]);
    return h * 60 + m;
}

save_btn.onclick = () => {
    const s = input_soil.value;
    if (s > 100 || s < 0) {
        alert("giá trị đổ ẩm không hợp lệ! (0 - 100)");
        return;
    }
    var lst_v = [];
    var lst_t = [];
    for (t of list_timer) {
        lst_t.push(t.value ? t.value : "chưa đặt");
        lst_v.push(stringTimeAsMinute(t.value));
    }
    request("POST", "/api/set", "json", (data) => {
        if (data.status == 0){
            setup_soil_value.textContent = s + "%";
            for (let i = 0; i < 5; i++){
                list_timer_value[i].textContent = lst_t[i];
            }
            alert("Lưu thành công");
        } else {
            alert("Lưu thất bại");
        }
    }, {
        soil: s ? parseFloat(s) : 100.0,
        timer: lst_v
    });
};

function getDataOnload(){
    request("GET", "/api/get", "json", (data) => {
        if (data.status == 0){
            updateData(data);
            input_soil.value = data.soil_set;
            for (let i = 0; i < 5; i++) {
                list_timer[i].value = list_timer_value[i].textContent;
            }
        }
    });
}

document.addEventListener("DOMContentLoaded", () => {
    // console.log("start");
    getDataOnload();
});