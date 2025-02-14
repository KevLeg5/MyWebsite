function openTab(evt, tabName) {
    // Save the selected tab to localStorage
    localStorage.setItem('selectedTab', tabName);

    // Reload the page
    window.location.reload();
}

// On page load, open the saved tab
window.onload = function () {
    var selectedTab = localStorage.getItem('selectedTab') || 'Home'; // Default to 'Home' if no tab is saved
    var tabcontent = document.getElementsByClassName("tabcontent");
    
    // Hide all tab content
    for (var i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }
    
    // Show the selected tab
    var activeTab = document.getElementById(selectedTab);
    if (activeTab) {
        activeTab.style.display = "block";
    }

    // Set the active class on the corresponding button
    var tablinks = document.getElementsByClassName("tablinks");
    for (var i = 0; i < tablinks.length; i++) {
        if (tablinks[i].getAttribute('onclick').includes(selectedTab)) {
            tablinks[i].classList.add("active");
        } else {
            tablinks[i].classList.remove("active");
        }
    }

    // Clear the saved tab from localStorage
    localStorage.removeItem('selectedTab');
};