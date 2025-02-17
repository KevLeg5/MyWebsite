function openTab(evt, tabName) {
    // Save the selected tab to localStorage
    localStorage.setItem('selectedTab', tabName);

    // Hide all tab content
    var tabcontent = document.getElementsByClassName("tabcontent");
    for (var i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    // Show the selected tab content
    var activeTab = document.getElementById(tabName);
    if (activeTab) {
        activeTab.style.display = "block";
    }

    // Remove the "active" class from all tab buttons
    var tablinks = document.getElementsByClassName("tablinks");
    for (var i = 0; i < tablinks.length; i++) {
        tablinks[i].classList.remove("active");
    }

    // Add the "active" class to the clicked button
    evt.currentTarget.classList.add("active");
}

// On page load, open the saved tab
window.onload = function () {
    var selectedTab = localStorage.getItem('selectedTab') || 'Home'; // Default to 'Home' if no tab is saved

    // Hide all tab content
    var tabcontent = document.getElementsByClassName("tabcontent");
    for (var i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    // Show the selected tab content
    var activeTab = document.getElementById(selectedTab);
    if (activeTab) {
        activeTab.style.display = "block";
    }

    // Set the "active" class on the corresponding button
    var tablinks = document.getElementsByClassName("tablinks");
    for (var i = 0; i < tablinks.length; i++) {
        if (tablinks[i].getAttribute('onclick').includes(selectedTab)) {
            tablinks[i].classList.add("active");
        } else {
            tablinks[i].classList.remove("active");
        }
    }
};