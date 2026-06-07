import {
  Color,
  WebGLRenderer,
  Scene,
  PerspectiveCamera,
  Mesh,
  BoxGeometry,
  MeshStandardMaterial,
  TextureLoader,
  AmbientLight,
  DirectionalLight,
  GridHelper,
} from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js'
import Stats from 'stats-js'

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

    // Cargar la textura desde public/textures/uv_grid.png para la Fase 1
    const loader = new TextureLoader()
    loader.load('./textures/uv_grid.png', (texture) => {
      this.setCube(texture)
      this.handleResize()
      this.events()
    })
  }

  setRender() {
    this.#renderer = new WebGLRenderer({
      canvas: this.#canvas,
      antialias: true,
    })
  }

  setScene() {
    this.#scene = new Scene()
    this.#scene.background = new Color(0x121214) // Slate oscuro premium
  }

  setCamera() {
    const aspectRatio = this.#width / this.#height
    const fieldOfView = 50
    const nearPlane = 0.1
    const farPlane = 1000

    this.#camera = new PerspectiveCamera(fieldOfView, aspectRatio, nearPlane, farPlane)
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

  setCube(texture) {
    const geometry = new BoxGeometry(1.8, 1.8, 1.8)
    const material = new MeshStandardMaterial({
      map: texture,
      roughness: 0.4,
      metalness: 0.1
    })

    this.#mesh = new Mesh(geometry, material)
    this.#scene.add(this.#mesh)
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
}
