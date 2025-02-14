class CircularCarousel {
  constructor(container, images) {
    this.container = typeof container === 'string' ? document.querySelector(container) : container;
    if (!this.container) {
      throw new Error('Container element not found');
    }
    
    // Create track if it doesn't exist
    this.track = this.container.querySelector('.carousel-track');
    if (!this.track) {
      this.track = document.createElement('div');
      this.track.className = 'carousel-track';
      this.container.appendChild(this.track);
    }
    
    if (!Array.isArray(images) || images.length === 0) {
      throw new Error('Images array must be provided and contain at least one image');
    }
    
    this.images = images;
    this.currentIndex = 0;
    this.init();
  }

  init() {
    // Clear existing slides
    this.track.innerHTML = '';
    
    // Create slides
    this.images.forEach((src, index) => {
      const slide = document.createElement('div');
      slide.className = 'carousel-slide';
      slide.innerHTML = `<img src="${src}" alt="Image ${index + 1}">`;
      
      slide.addEventListener('click', () => {
        if (index !== this.currentIndex) {
          this.goToSlide(index);
        }
      });
      
      this.track.appendChild(slide);
    });
    this.updateSlides();
  }

  getCircularIndex(index) {
    if (index < 0) return this.images.length - 1;
    if (index >= this.images.length) return 0;
    return index;
  }

  updateSlides() {
    const slides = this.track.querySelectorAll('.carousel-slide');
    slides.forEach((slide) => {
      slide.className = 'carousel-slide';
    });
    
    // Only show and position the three visible slides
    const currentSlide = slides[this.currentIndex];
    const prevSlide = slides[this.getCircularIndex(this.currentIndex - 1)];
    const nextSlide = slides[this.getCircularIndex(this.currentIndex + 1)];
    
    currentSlide.classList.add('current');
    prevSlide.classList.add('preview-left');
    nextSlide.classList.add('preview-right');
  }

  goToSlide(index) {
    this.currentIndex = index;
    this.updateSlides();
  }
}

// Make the class available globally
window.CircularCarousel = CircularCarousel;