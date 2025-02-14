document.querySelectorAll('.tab-header').forEach(header => {
    header.addEventListener('click', () => {
        const tab = header.parentElement;
        tab.classList.toggle('active');
    });
});

