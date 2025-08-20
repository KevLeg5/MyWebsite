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

    // Initialize scroll indicator functionality
    if (selectedTab === 'Home') {
        initScrollIndicator();
    }
};

// Scroll indicator functionality
function initScrollIndicator() {
    const scrollIndicator = document.getElementById('scrollIndicator');
    const homeTab = document.getElementById('Home');
    
    if (!scrollIndicator || !homeTab) {
        console.log('Scroll indicator or home tab not found');
        return;
    }
    
    // Remove any existing scroll listeners to prevent duplicates
    if (window.scrollHandler) {
        window.removeEventListener('scroll', window.scrollHandler);
        document.removeEventListener('scroll', window.scrollHandler);
        homeTab.removeEventListener('scroll', window.scrollHandler);
    }
    
    // Define the scroll handler with debugging
    window.scrollHandler = function() {
        const scrollTop = window.pageYOffset || document.documentElement.scrollTop || document.body.scrollTop || 0;
        console.log('Scroll detected:', scrollTop); // Debug line
        
        if (scrollTop <= 50) {
            scrollIndicator.classList.add('show');
            console.log('Showing indicator'); // Debug line
        } else {
            scrollIndicator.classList.remove('show');
            console.log('Hiding indicator'); // Debug line
        }
    };
    
    // Add scroll listeners to multiple elements to catch all scroll events
    window.addEventListener('scroll', window.scrollHandler, { passive: true });
    document.addEventListener('scroll', window.scrollHandler, { passive: true });
    homeTab.addEventListener('scroll', window.scrollHandler, { passive: true });
    
    // Also try listening to the body and document element
    if (document.body) {
        document.body.addEventListener('scroll', window.scrollHandler, { passive: true });
    }
    if (document.documentElement) {
        document.documentElement.addEventListener('scroll', window.scrollHandler, { passive: true });
    }
    
    // Show indicator initially with a delay
    setTimeout(() => {
        const scrollTop = window.pageYOffset || document.documentElement.scrollTop || document.body.scrollTop || 0;
        console.log('Initial scroll position:', scrollTop); // Debug line
        if (scrollTop <= 50) {
            scrollIndicator.classList.add('show');
        }
    }, 1000);
}