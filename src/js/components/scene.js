import {
  Color,
  WebGLRenderer,
  Scene,
  PerspectiveCamera,
  Mesh,
  BoxGeometry,
  SphereGeometry,
  PlaneGeometry,
  MeshStandardMaterial,
  AmbientLight,
  DirectionalLight,
  PointLight,
  GridHelper,
  RepeatWrapping,
  ClampToEdgeWrapping,
  NearestFilter,
  LinearFilter,
  NearestMipmapNearestFilter,
  NearestMipmapLinearFilter,
  LinearMipmapNearestFilter,
  LinearMipmapLinearFilter
} from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js'
import Stats from 'stats-js'
import GUI from 'lil-gui'
import TextureManager from '../textures.js'

// Mapeos de strings de configuración a constantes de Three.js
const WRAPPING_MODES = {
  'RepeatWrapping': RepeatWrapping,
  'ClampToEdgeWrapping': ClampToEdgeWrapping
}

const FILTER_MODES = {
  'NearestFilter': NearestFilter,
  'LinearFilter': LinearFilter,
  'NearestMipmapNearestFilter': NearestMipmapNearestFilter,
  'NearestMipmapLinearFilter': NearestMipmapLinearFilter,
  'LinearMipmapNearestFilter': LinearMipmapNearestFilter,
  'LinearMipmapLinearFilter': LinearMipmapLinearFilter
}

export default class MainScene {
  #canvas
  #renderer
  #scene
  #camera
  #controls
  #stats
  #width
  #height
  #mesh
  #gui
  
  // Estado de los parámetros interactivos
  state = {
    geometry: 'cube',          // cube, sphere, plane
    textureName: 'uv_grid',    // uv_grid, brick, wood, stone
    textureActive: true,
    wrapMode: 'RepeatWrapping',
    repeatX: 1,
    repeatY: 1,
    minFilter: 'LinearMipmapLinearFilter',
    magFilter: 'LinearFilter',
    generateMipmaps: true,
    anisotropy: 1,
    color: '#3b82f6'           // Color plano usado cuando se apaga la textura
  }

  constructor() {
    this.#canvas = document.querySelector('.scene')
    this.init()
  }

  init = async () => {
    // Cargar todas las texturas de manera asíncrona
    await TextureManager.loadAll()

    this.setStats()
    this.setScene()
    this.setRender()
    this.setCamera()
    this.setLights()
    this.setControls()
    this.setGridHelper()

    // Obtener la anisotropía máxima soportada por la GPU del cliente
    this.maxAnisotropy = this.#renderer.capabilities.getMaxAnisotropy()
    this.state.anisotropy = 1 // Por defecto a 1 (sin anisotropía)

    this.setMaterial()
    this.setGeometry(this.state.geometry)
    this.updateMaterialTexture()

    this.setGUI()
    this.handleResize()

    // Registrar eventos (resize y RAF)
    this.events()

    // Registrar un listener global para llamadas desde la interfaz HTML
    window.sceneInstance = this
  }

  /**
   * Configuración del renderizador WebGL
   * Habilitamos antialiasing y permitimos leer el buffer para capturas de pantalla
   */
  setRender() {
    this.#renderer = new WebGLRenderer({
      canvas: this.#canvas,
      antialias: true,
      preserveDrawingBuffer: true // Permite capturar capturas de pantalla de forma síncrona
    })
  }

  /**
   * Escena con fondo gris oscuro para resaltar los objetos 3D y texturas
   */
  setScene() {
    this.#scene = new Scene()
    this.#scene.background = new Color(0x121214) // Slate oscuro premium
  }

  /**
   * Cámara de perspectiva para simular la visión humana
   */
  setCamera() {
    const aspectRatio = this.#width / this.#height
    const fieldOfView = 50
    const nearPlane = 0.1
    const farPlane = 1000

    this.#camera = new PerspectiveCamera(fieldOfView, aspectRatio, nearPlane, farPlane)
    this.#camera.position.set(4, 3, 5) // Vista en ángulo diagonal
    this.#camera.lookAt(0, 0, 0)

    this.#scene.add(this.#camera)
  }

  /**
   * Luces para dar volumen y realismo a las geometrías
   */
  setLights() {
    // Luz ambiental para iluminar las zonas en sombra
    const ambientLight = new AmbientLight(0xffffff, 0.4)
    this.#scene.add(ambientLight)

    // Luz direccional que simula el Sol, generando sombreado y brillos
    const directionalLight = new DirectionalLight(0xffffff, 0.8)
    directionalLight.position.set(5, 8, 5)
    this.#scene.add(directionalLight)

    // Luz de relleno sutil para realzar los contornos del objeto
    const pointLight = new PointLight(0xffffff, 0.3, 15)
    pointLight.position.set(-4, 2, -4)
    this.#scene.add(pointLight)
  }

  /**
   * OrbitControls para navegar por la escena
   */
  setControls() {
    this.#controls = new OrbitControls(this.#camera, this.#renderer.domElement)
    this.#controls.enableDamping = true // Suavizado de movimiento
    this.#controls.dampingFactor = 0.05
    this.#controls.minDistance = 2
    this.#controls.maxDistance = 15
  }

  /**
   * Grid para ubicar los objetos en el espacio 3D
   */
  setGridHelper() {
    const gridHelper = new GridHelper(10, 10, 0x4f46e5, 0x27272a)
    gridHelper.position.y = -1.5
    this.#scene.add(gridHelper)
  }

  /**
   * Inicializa el material utilizando MeshStandardMaterial
   * que reacciona a la luz y soporta mapeo de texturas.
   */
  setMaterial() {
    this.material = new MeshStandardMaterial({
      color: new Color(this.state.color),
      roughness: 0.4,
      metalness: 0.1
    })
  }

  /**
   * Cambia dinámicamente la geometría activa eliminando la anterior de la memoria
   */
  setGeometry(type) {
    if (this.#mesh) {
      this.#scene.remove(this.#mesh)
      this.#mesh.geometry.dispose() // Liberar memoria GPU
    }

    let geometry
    if (type === 'cube') {
      // Cubo estándar de 2x2x2
      geometry = new BoxGeometry(2, 2, 2)
    } else if (type === 'sphere') {
      // Esfera con subdivisiones altas para suavidad y mapeo esférico correcto
      geometry = new SphereGeometry(1.3, 64, 64)
    } else if (type === 'plane') {
      // Plano rectangular para mostrar efectos de filtrado y repetición plana (e.g. suelo)
      geometry = new PlaneGeometry(2.5, 2.5, 32, 32)
    }

    this.#mesh = new Mesh(geometry, this.material)
    this.#scene.add(this.#mesh)
    this.state.geometry = type
    
    this.notifyUI()
  }

  /**
   * Aplica los parámetros del estado a la textura activa y actualiza el material
   */
  updateMaterialTexture() {
    // Si la textura está desactivada o seleccionamos 'none' (color plano)
    if (!this.state.textureActive || this.state.textureName === 'none') {
      this.material.map = null
    } else {
      const texture = TextureManager.getTexture(this.state.textureName)
      if (texture) {
        // Mapear constantes
        const wrapVal = WRAPPING_MODES[this.state.wrapMode]
        const minFilterVal = FILTER_MODES[this.state.minFilter]
        const magFilterVal = FILTER_MODES[this.state.magFilter]

        // 1. Configurar Wrapping y repetición
        TextureManager.updateWrapping(texture, wrapVal, this.state.repeatX, this.state.repeatY)

        // 2. Configurar Filtros y generación de Mipmaps
        TextureManager.updateFilters(texture, minFilterVal, magFilterVal, this.state.generateMipmaps)

        // 3. Configurar Anisotropía
        TextureManager.updateAnisotropy(texture, this.state.anisotropy, this.maxAnisotropy)

        // Asignar al material
        this.material.map = texture
      }
    }

    // Indicar al material que debe actualizarse en el siguiente frame
    this.material.needsUpdate = true

    this.notifyUI()
  }

  /**
   * Informa al frontend sobre los cambios de estado a través de eventos personalizados
   */
  notifyUI() {
    const event = new CustomEvent('textureStateChanged', { detail: this.state })
    window.dispatchEvent(event)
  }

  /**
   * Crea y estiliza el panel lil-gui interactivo
   */
  setGUI() {
    this.#gui = new GUI({ title: 'Configuración Gráfica' })
    
    // Carpeta de Objeto
    const objFolder = this.#gui.addFolder('Objeto y Geometría')
    objFolder.add(this.state, 'geometry', { 'Cubo': 'cube', 'Esfera': 'sphere', 'Plano': 'plane' })
      .name('Geometría')
      .onChange((val) => this.setGeometry(val))
    objFolder.add(this.state, 'color').name('Color Base').onChange((val) => {
      this.material.color.set(val)
    })

    // Carpeta de Textura
    const texFolder = this.#gui.addFolder('Control de Textura')
    texFolder.add(this.state, 'textureActive').name('Textura Activa').onChange(() => this.updateMaterialTexture())
    texFolder.add(this.state, 'textureName', {
      'UV Grid (Mapeo)': 'uv_grid',
      'Ladrillo (Patrón)': 'brick',
      'Madera (Orgánico)': 'wood',
      'Piedra (Rugoso)': 'stone'
    }).name('Textura').onChange(() => this.updateMaterialTexture())

    // Carpeta de Wrapping
    const wrapFolder = this.#gui.addFolder('Envoltura y Repetición')
    wrapFolder.add(this.state, 'wrapMode', ['RepeatWrapping', 'ClampToEdgeWrapping']).name('Modo Envoltura').onChange(() => this.updateMaterialTexture())
    wrapFolder.add(this.state, 'repeatX', 0.1, 10, 0.1).name('Repetición U (X)').onChange(() => this.updateMaterialTexture())
    wrapFolder.add(this.state, 'repeatY', 0.1, 10, 0.1).name('Repetición V (Y)').onChange(() => this.updateMaterialTexture())

    // Carpeta de Filtrado
    const filterFolder = this.#gui.addFolder('Filtros y Mipmapping')
    filterFolder.add(this.state, 'generateMipmaps').name('Generar Mipmaps').onChange(() => this.updateMaterialTexture())
    filterFolder.add(this.state, 'minFilter', [
      'NearestFilter',
      'LinearFilter',
      'NearestMipmapNearestFilter',
      'NearestMipmapLinearFilter',
      'LinearMipmapNearestFilter',
      'LinearMipmapLinearFilter'
    ]).name('Filtro Min (Lejos)').onChange(() => this.updateMaterialTexture())
    filterFolder.add(this.state, 'magFilter', ['NearestFilter', 'LinearFilter']).name('Filtro Mag (Cerca)').onChange(() => this.updateMaterialTexture())
    
    // Anisotropía (desde 1 hasta el máximo soportado por la GPU)
    filterFolder.add(this.state, 'anisotropy', 1, this.maxAnisotropy, 1).name('Anisotropía').onChange(() => this.updateMaterialTexture())

    // Añadir botón de captura directo en GUI
    const controllerObj = {
      screenshot: () => this.captureScreenshot()
    }
    this.#gui.add(controllerObj, 'screenshot').name('Tomar Captura PNG')

    this.#gui.open()
  }

  /**
   * Captura el estado actual del canvas y lo descarga como PNG
   */
  captureScreenshot() {
    // Renderizar para asegurar que el buffer de WebGL esté fresco
    this.#renderer.render(this.#scene, this.#camera)
    const dataURL = this.#canvas.toDataURL('image/png')
    
    const link = document.createElement('a')
    link.download = `captura_textura_${this.state.geometry}_${this.state.textureName}.png`
    link.href = dataURL
    link.click()
  }

  /**
   * Monitor de estadísticas FPS
   */
  setStats() {
    this.#stats = new Stats()
    this.#stats.showPanel(0)
    
    // Estilizar contenedor de stats para encajar en el layout oscuro
    this.#stats.dom.style.position = 'absolute'
    this.#stats.dom.style.top = '10px'
    this.#stats.dom.style.left = '10px'
    this.#stats.dom.style.zIndex = '100'
    
    document.body.appendChild(this.#stats.dom)
  }

  events() {
    window.addEventListener('resize', this.handleResize, { passive: true })
    this.draw()
  }

  /**
   * Loop de renderizado
   */
  draw = () => {
    this.#stats.begin()

    if (this.#controls) this.#controls.update()
    this.#renderer.render(this.#scene, this.#camera)

    this.#stats.end()
    this.raf = window.requestAnimationFrame(this.draw)
  }

  handleResize = () => {
    this.#width = window.innerWidth
    this.#height = window.innerHeight

    this.#camera.aspect = this.#width / this.#height
    this.#camera.updateProjectionMatrix()

    const DPR = window.devicePixelRatio ? Math.min(window.devicePixelRatio, 2) : 1
    this.#renderer.setPixelRatio(DPR)
    this.#renderer.setSize(this.#width, this.#height)
  }

  // Permite interactuar programáticamente con el estado desde el HTML
  updateStateProperty(key, value) {
    if (this.state[key] !== undefined) {
      this.state[key] = value
      
      // Si cambia geometría
      if (key === 'geometry') {
        this.setGeometry(value)
      } else {
        // Cualquier otro parámetro de textura requiere recargar textura
        this.updateMaterialTexture()
      }

      // Sincronizar lil-gui si existe
      if (this.#gui) {
        this.#gui.controllersRecursive().forEach(controller => {
          if (controller.property === key) {
            controller.setValue(value)
          }
        })
      }
    }
  }
}
