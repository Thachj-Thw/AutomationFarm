const navbar = document.querySelector("#navbar");
const navbarBtns = document.querySelectorAll("#navbar > a");
const container = document.querySelectorAll("#container > div");
const dashboardContainer = document.querySelector(".dashboard-container");
const settingContainer = document.querySelector(".setting-container");
const dashboardButton = document.querySelector("#dashboard-button");
const settingButton = document.querySelector("#setting-button");
navbar.onmouseleave = (e) => {navbar.classList.add("close")};
navbar.onmouseenter = (e) => {navbar.classList.remove("close")};
navbarBtns.forEach((e) => {
    e.addEventListener("click", () => {
        navbarBtns.forEach((b) => {b.classList.remove("select")});
        container.forEach((b) => b.classList.add("hidden"));
        e.classList.add("select");
        if (e == dashboardButton){
            dashboardContainer.classList.remove("hidden");
        }else if (e == settingButton){
            settingContainer.classList.remove("hidden");
        }
    });
    }
);