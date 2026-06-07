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
  GridHelper,
  TextureLoader,
} from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js'
import Stats from 'stats-js'
import GUI from 'lil-gui'

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

  // Estado de los parámetros para la Fase 2
  state = {
    geometry: 'cube',          // cube, sphere, plane
    textureName: 'uv_grid',    // uv_grid, brick, wood, stone
    textureActive: true,
    color: '#3b82f6',
    // Parámetros de la Fase 3 inicializados por defecto
    wrapMode: 'RepeatWrapping',
    repeatX: 1,
    repeatY: 1,
    minFilter: 'LinearMipmapLinearFilter',
    magFilter: 'LinearFilter',
    generateMipmaps: true,
    anisotropy: 1
  }

  textures = {}

  constructor() {
    this.#canvas = document.querySelector('.scene')
    this.init()
  }

  init = async () => {
    this.setStats()
    this.setScene()
    this.setRender()
    this.setCamera()
    this.setLights()
    this.setControls()
    this.setGridHelper()

    // Cargar texturas de la Fase 2
    await this.loadTextures()

    this.setMaterial()
    this.setGeometry(this.state.geometry)
    this.updateMaterialTexture()

    this.setGUI()
    this.handleResize()
    this.events()

    window.sceneInstance = this
  }

  async loadTextures() {
    const loader = new TextureLoader()
    const paths = {
      uv_grid: './textures/uv_grid.png',
      brick: './textures/brick.png',
      wood: './textures/wood.png',
      stone: './textures/stone.png'
    }

    const promises = Object.entries(paths).map(([key, path]) => {
      return new Promise((resolve) => {
        loader.load(path, (tex) => {
          this.textures[key] = tex
          resolve()
        })
      })
    })

    await Promise.all(promises)
  }

  setRender() {
    this.#renderer = new WebGLRenderer({
      canvas: this.#canvas,
      antialias: true,
    })
  }

  setScene() {
    this.#scene = new Scene()
    this.#scene.background = new Color(0x121214)
  }

  setCamera() {
    const aspectRatio = this.#width / this.#height
    this.#camera = new PerspectiveCamera(50, aspectRatio, 0.1, 1000)
    this.#camera.position.set(3, 3, 4)
    this.#camera.lookAt(0, 0, 0)
    this.#scene.add(this.#camera)
  }

  setLights() {
    const ambientLight = new AmbientLight(0xffffff, 0.4)
    this.#scene.add(ambientLight)

    const directionalLight = new DirectionalLight(0xffffff, 0.8)
    directionalLight.position.set(5, 8, 5)
    this.#scene.add(directionalLight)
  }

  setControls() {
    this.#controls = new OrbitControls(this.#camera, this.#renderer.domElement)
    this.#controls.enableDamping = true
    this.#controls.dampingFactor = 0.05
  }

  setGridHelper() {
    const gridHelper = new GridHelper(10, 10, 0x4f46e5, 0x27272a)
    gridHelper.position.y = -1.2
    this.#scene.add(gridHelper)
  }

  setMaterial() {
    this.material = new MeshStandardMaterial({
      color: new Color(this.state.color),
      roughness: 0.4,
      metalness: 0.1
    })
  }

  setGeometry(type) {
    if (this.#mesh) {
      this.#scene.remove(this.#mesh)
      this.#mesh.geometry.dispose()
    }

    let geometry
    if (type === 'cube') {
      geometry = new BoxGeometry(1.8, 1.8, 1.8)
    } else if (type === 'sphere') {
      geometry = new SphereGeometry(1.2, 64, 64)
    } else if (type === 'plane') {
      geometry = new PlaneGeometry(2.2, 2.2)
    }

    this.#mesh = new Mesh(geometry, this.material)
    this.#scene.add(this.#mesh)
    this.state.geometry = type

    this.notifyUI()
  }

  updateMaterialTexture() {
    if (!this.state.textureActive || this.state.textureName === 'none') {
      this.material.map = null
    } else {
      this.material.map = this.textures[this.state.textureName] || null
    }
    this.material.needsUpdate = true
    this.notifyUI()
  }

  notifyUI() {
    const event = new CustomEvent('textureStateChanged', { detail: this.state })
    window.dispatchEvent(event)
  }

  setGUI() {
    this.#gui = new GUI({ title: 'Controles - Fase 2' })

    const objFolder = this.#gui.addFolder('Geometría')
    objFolder.add(this.state, 'geometry', { 'Cubo': 'cube', 'Esfera': 'sphere', 'Plano': 'plane' })
      .name('Objeto')
      .onChange((val) => this.setGeometry(val))
    objFolder.add(this.state, 'color').name('Color Base').onChange((val) => {
      this.material.color.set(val)
    })

    const texFolder = this.#gui.addFolder('Textura')
    texFolder.add(this.state, 'textureActive').name('Textura Activa').onChange(() => this.updateMaterialTexture())
    texFolder.add(this.state, 'textureName', {
      'UV Grid': 'uv_grid',
      'Ladrillo': 'brick',
      'Madera': 'wood',
      'Piedra': 'stone'
    }).name('Seleccionar Textura').onChange(() => this.updateMaterialTexture())

    this.#gui.open()
  }

  setStats() {
    this.#stats = new Stats()
    this.#stats.showPanel(0)
    document.body.appendChild(this.#stats.dom)
  }

  events() {
    window.addEventListener('resize', this.handleResize, { passive: true })
    this.draw()
  }

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

  updateStateProperty(key, value) {
    if (this.state[key] !== undefined) {
      this.state[key] = value
      if (key === 'geometry') {
        this.setGeometry(value)
      } else {
        this.updateMaterialTexture()
      }
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
