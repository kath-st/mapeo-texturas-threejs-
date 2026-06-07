/**
 * UIManager
 * Gestiona la interactividad del panel de control HTML,
 * la navegación de pestañas educativas y la sincronización con la escena 3D.
 */
class UIManager {
  constructor() {
    this.tabs = document.querySelectorAll('.dashboard__tab-btn')
    this.sections = document.querySelectorAll('.dashboard__concept-section')
    this.toggleBtn = document.getElementById('toggle-dashboard')
    this.dashboard = document.querySelector('.dashboard')
    this.screenshotBtn = document.getElementById('btn-screenshot')
    this.uploadBtn = document.getElementById('btn-upload')
    this.fileInput = document.getElementById('texture-upload')
    this.stateValues = {
      geometry: document.getElementById('val-geometry'),
      texture: document.getElementById('val-texture'),
      wrap: document.getElementById('val-wrap'),
      repeat: document.getElementById('val-repeat'),
      filterMin: document.getElementById('val-filter-min'),
      filterMag: document.getElementById('val-filter-mag'),
      mipmaps: document.getElementById('val-mipmaps'),
      anisotropy: document.getElementById('val-anisotropy')
    }

    this.init()
  }

  init() {
    // 1. Manejo de Pestañas Educativas
    this.tabs.forEach(tab => {
      tab.addEventListener('click', () => {
        const targetSection = tab.getAttribute('data-tab')
        this.switchTab(tab, targetSection)
      })
    })

    // 2. Colapsar / Expandir Dashboard
    if (this.toggleBtn && this.dashboard) {
      this.toggleBtn.addEventListener('click', () => {
        this.dashboard.classList.toggle('dashboard--collapsed')
        // Cambiar el texto o ícono del botón
        if (this.dashboard.classList.contains('dashboard--collapsed')) {
          this.toggleBtn.innerHTML = '⚡ Mostrar Guía'
          this.toggleBtn.classList.add('toggle-btn--floating')
        } else {
          this.toggleBtn.innerHTML = '✕ Ocultar Guía'
          this.toggleBtn.classList.remove('toggle-btn--floating')
        }
      })
    }

    // 3. Captura de Pantalla
    if (this.screenshotBtn) {
      this.screenshotBtn.addEventListener('click', () => {
        if (window.sceneInstance) {
          window.sceneInstance.captureScreenshot()
        }
      })
    }

    // 3.5. Subida de Imagen Personalizada
    if (this.uploadBtn && this.fileInput) {
      this.uploadBtn.addEventListener('click', () => {
        this.fileInput.click()
      })
      this.fileInput.addEventListener('change', (e) => {
        const file = e.target.files[0]
        if (file && window.sceneInstance) {
          const url = URL.createObjectURL(file)
          window.sceneInstance.loadCustomTexture(url)
        }
      })
    }

    // 4. Escuchar eventos de la escena 3D para actualizar el inspector
    window.addEventListener('textureStateChanged', (e) => {
      this.updateStateInspector(e.detail)
    })

    // 5. Botones de Presets / Experimentos rápidos en el HTML
    this.setupPresets()
  }

  /**
   * Cambia la pestaña de información activa
   */
  switchTab(activeTabBtn, targetSectionId) {
    // Desactivar todos los botones y secciones
    this.tabs.forEach(btn => btn.classList.remove('dashboard__tab-btn--active'))
    this.sections.forEach(sec => sec.classList.remove('dashboard__concept-section--active'))

    // Activar botón seleccionado
    activeTabBtn.classList.add('dashboard__tab-btn--active')

    // Activar sección seleccionada
    const targetSec = document.getElementById(`concept-${targetSectionId}`)
    if (targetSec) {
      targetSec.classList.add('dashboard__concept-section--active')
    }
  }

  /**
   * Actualiza los valores de texto en el panel "Inspector de Estado" en tiempo real
   */
  updateStateInspector(state) {
    if (!state) return

    if (this.stateValues.geometry) this.stateValues.geometry.textContent = this.formatName(state.geometry)
    
    if (this.stateValues.texture) {
      this.stateValues.texture.textContent = state.textureActive 
        ? this.formatName(state.textureName) 
        : 'Desactivada (Color Plano)'
    }

    if (this.stateValues.wrap) this.stateValues.wrap.textContent = state.wrapMode
    
    if (this.stateValues.repeat) {
      this.stateValues.repeat.textContent = `U: ${state.repeatX.toFixed(1)}, V: ${state.repeatY.toFixed(1)}`
    }

    if (this.stateValues.filterMin) this.stateValues.filterMin.textContent = state.minFilter
    if (this.stateValues.filterMag) this.stateValues.filterMag.textContent = state.magFilter
    if (this.stateValues.mipmaps) {
      this.stateValues.mipmaps.textContent = state.generateMipmaps ? 'Activado' : 'Desactivado'
      this.stateValues.mipmaps.className = state.generateMipmaps ? 'badge badge--success' : 'badge badge--danger'
    }
    
    if (this.stateValues.anisotropy) {
      this.stateValues.anisotropy.textContent = `${state.anisotropy}x`
    }
  }

  /**
   * Da formato legible a las variables internas
   */
  formatName(key) {
    const names = {
      cube: 'Cubo 3D',
      sphere: 'Esfera 3D',
      plane: 'Plano 2D',
      uv_grid: 'UV Grid (Mapeo)',
      brick: 'Ladrillo',
      wood: 'Madera',
      stone: 'Piedra',
      custom: 'Imagen Subida (Custom)'
    }
    return names[key] || key
  }

  /**
   * Enlaza botones rápidos de experimentación con la instancia 3D
   */
  setupPresets() {
    const presetButtons = document.querySelectorAll('[data-preset]')
    presetButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        const presetType = btn.getAttribute('data-preset')
        this.applyPreset(presetType)
      })
    })
  }

  /**
   * Aplica presets diseñados para demostrar fenómenos específicos de texturizado
   */
  applyPreset(preset) {
    if (!window.sceneInstance) return

    const scene = window.sceneInstance

    switch (preset) {
      case 'distorsion-uv':
        // Demuestra cómo el mapeo UV se adapta a diferentes geometrías
        scene.updateStateProperty('textureActive', true)
        scene.updateStateProperty('textureName', 'uv_grid')
        scene.updateStateProperty('wrapMode', 'RepeatWrapping')
        scene.updateStateProperty('repeatX', 1)
        scene.updateStateProperty('repeatY', 1)
        this.switchTab(this.tabs[1], 'uv') // Ir a pestaña UV
        break

      case 'wrapping-clamp':
        // Demuestra el efecto de ClampToEdge al estirar la textura en bordes
        scene.updateStateProperty('textureActive', true)
        scene.updateStateProperty('textureName', 'brick')
        scene.updateStateProperty('wrapMode', 'ClampToEdgeWrapping')
        scene.updateStateProperty('repeatX', 3)
        scene.updateStateProperty('repeatY', 3)
        this.switchTab(this.tabs[2], 'wrapping') // Ir a pestaña Wrapping
        break

      case 'wrapping-repeat':
        // Demuestra el patrón repetido de ladrillos
        scene.updateStateProperty('textureActive', true)
        scene.updateStateProperty('textureName', 'brick')
        scene.updateStateProperty('wrapMode', 'RepeatWrapping')
        scene.updateStateProperty('repeatX', 4)
        scene.updateStateProperty('repeatY', 4)
        this.switchTab(this.tabs[2], 'wrapping') // Ir a pestaña Wrapping
        break

      case 'filter-nearest':
        // Demuestra pixelado al acercar o aliasing al alejar sin mipmaps
        scene.updateStateProperty('textureActive', true)
        scene.updateStateProperty('textureName', 'uv_grid')
        scene.updateStateProperty('minFilter', 'NearestFilter')
        scene.updateStateProperty('magFilter', 'NearestFilter')
        scene.updateStateProperty('generateMipmaps', false)
        this.switchTab(this.tabs[3], 'filtering') // Ir a pestaña Filtros
        break

      case 'filter-linear':
        // Demuestra suavizado bilineal
        scene.updateStateProperty('textureActive', true)
        scene.updateStateProperty('textureName', 'uv_grid')
        scene.updateStateProperty('minFilter', 'LinearFilter')
        scene.updateStateProperty('magFilter', 'LinearFilter')
        scene.updateStateProperty('generateMipmaps', false)
        this.switchTab(this.tabs[3], 'filtering')
        break

      case 'anisotropy-on':
        // Muestra textura clara a lo lejos en plano inclinado
        scene.updateStateProperty('geometry', 'plane')
        scene.updateStateProperty('textureActive', true)
        scene.updateStateProperty('textureName', 'stone')
        scene.updateStateProperty('wrapMode', 'RepeatWrapping')
        scene.updateStateProperty('repeatX', 8)
        scene.updateStateProperty('repeatY', 8)
        scene.updateStateProperty('minFilter', 'LinearMipmapLinearFilter')
        scene.updateStateProperty('generateMipmaps', true)
        scene.updateStateProperty('anisotropy', scene.maxAnisotropy)
        this.switchTab(this.tabs[4], 'anisotropy')
        break

      case 'anisotropy-off':
        // Muestra textura borrosa a lo lejos en plano inclinado
        scene.updateStateProperty('geometry', 'plane')
        scene.updateStateProperty('textureActive', true)
        scene.updateStateProperty('textureName', 'stone')
        scene.updateStateProperty('wrapMode', 'RepeatWrapping')
        scene.updateStateProperty('repeatX', 8)
        scene.updateStateProperty('repeatY', 8)
        scene.updateStateProperty('minFilter', 'LinearMipmapLinearFilter')
        scene.updateStateProperty('generateMipmaps', true)
        scene.updateStateProperty('anisotropy', 1)
        this.switchTab(this.tabs[4], 'anisotropy')
        break
    }
  }
}

// Iniciar cuando el DOM esté listo
document.addEventListener('DOMContentLoaded', () => {
  window.uiInstance = new UIManager()
})
