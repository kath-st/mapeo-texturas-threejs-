import {
  TextureLoader,
  RepeatWrapping,
  ClampToEdgeWrapping,
  NearestFilter,
  LinearFilter,
  NearestMipmapNearestFilter,
  NearestMipmapLinearFilter,
  LinearMipmapNearestFilter,
  LinearMipmapLinearFilter
} from 'three'

/**
 * TextureManager
 * Gestiona la carga y la configuración dinámica de las texturas de la aplicación.
 * Proporciona una interfaz para modificar filtros, envoltura, repetición y mipmapping
 * con explicaciones detalladas para los estudiantes de Computación Gráfica.
 */
class TextureManager {
  constructor() {
    this.loader = new TextureLoader()
    this.textures = {}
    
    // Rutas relativas a la carpeta public (servidas desde la raíz en Vite)
    this.texturePaths = {
      uv_grid: './textures/uv_grid.png',
      brick: './textures/brick.png',
      wood: './textures/wood.png',
      stone: './textures/stone.png'
    }

    // Opciones para lil-gui e interfaz de usuario
    this.wrappingOptions = {
      'RepeatWrapping (Repetir)': RepeatWrapping,
      'ClampToEdgeWrapping (Abrazar)': ClampToEdgeWrapping
    }

    this.filterOptions = {
      'NearestFilter (Vecino más Cercano)': NearestFilter,
      'LinearFilter (Bilineal)': LinearFilter,
      'NearestMipmapNearestFilter': NearestMipmapNearestFilter,
      'NearestMipmapLinearFilter': NearestMipmapLinearFilter,
      'LinearMipmapNearestFilter': LinearMipmapNearestFilter,
      'LinearMipmapLinearFilter (Predeterminado)': LinearMipmapLinearFilter
    }
  }

  /**
   * Carga todas las texturas de manera asíncrona
   * @returns {Promise} Resuelve cuando todas las texturas han sido cargadas.
   */
  loadAll() {
    const promises = Object.entries(this.texturePaths).map(([key, path]) => {
      return new Promise((resolve) => {
        this.loader.load(
          path,
          (texture) => {
            // Configuración inicial estándar para que se vean bien por defecto
            texture.wrapS = RepeatWrapping
            texture.wrapT = RepeatWrapping
            texture.minFilter = LinearMipmapLinearFilter
            texture.magFilter = LinearFilter
            texture.generateMipmaps = true
            
            this.textures[key] = texture
            
            // Inicializar la textura 'custom' con la cuadrícula por defecto como fallback seguro
            if (key === 'uv_grid') {
              this.textures['custom'] = texture
            }
            
            resolve(texture)
          },
          undefined,
          (error) => {
            console.error(`Error cargando la textura ${key} en la ruta ${path}:`, error)
            resolve(null) // Resuelve de todas formas para no romper la app
          }
        )
      })
    })

    return Promise.all(promises)
  }

  /**
   * Carga una textura personalizada a partir de un Blob URL de imagen local
   * @param {string} url - Blob URL temporal generado del archivo subido
   * @param {string} name - Nombre identificador de la textura
   * @returns {Promise}
   */
  loadCustomTexture(url, name = 'custom') {
    return new Promise((resolve) => {
      this.loader.load(url, (texture) => {
        // Configuración inicial estándar idéntica a las texturas fijas
        texture.wrapS = RepeatWrapping
        texture.wrapT = RepeatWrapping
        texture.minFilter = LinearMipmapLinearFilter
        texture.magFilter = LinearFilter
        texture.generateMipmaps = true

        this.textures[name] = texture
        resolve(texture)
      }, undefined, (err) => {
        console.error("Error al cargar la textura de imagen subida:", err)
        resolve(null)
      })
    })
  }

  /**
   * Obtiene una textura específica por su identificador
   * @param {string} key 
   * @returns {THREE.Texture|null}
   */
  getTexture(key) {
    return this.textures[key] || null
  }

  /**
   * Aplica la configuración de envoltura (Wrapping) a una textura.
   * - RepeatWrapping: La textura se repite indefinidamente.
   * - ClampToEdgeWrapping: El último píxel del borde se extiende hasta el infinito.
   */
  updateWrapping(texture, wrapMode, repeatX, repeatY) {
    if (!texture) return
    
    // wrapS define el comportamiento en el eje horizontal (U)
    // wrapT define el comportamiento en el eje vertical (V)
    texture.wrapS = wrapMode
    texture.wrapT = wrapMode

    // Configurar cuántas veces se repite la textura sobre el objeto
    texture.repeat.set(repeatX, repeatY)

    // Indicar a Three.js/WebGL que la textura ha cambiado y debe ser reenviada a la GPU
    texture.needsUpdate = true
  }

  /**
   * Actualiza los filtros de minificación y magnificación de la textura.
   * - Minification (minFilter): Se aplica cuando el objeto está lejos y la textura se reduce.
   * - Magnification (magFilter): Se aplica cuando el objeto está cerca y los píxeles de la textura se agrandan.
   */
  updateFilters(texture, minFilter, magFilter, generateMipmaps) {
    if (!texture) return

    texture.generateMipmaps = generateMipmaps
    texture.magFilter = magFilter
    texture.minFilter = minFilter

    // Corrección de seguridad: si mipmapping está desactivado y el filtro seleccionado requiere mipmaps,
    // se debe cambiar el filtro temporalmente o advertir, para evitar texturas negras o errores.
    // Los filtros con "Mipmap" en el nombre requieren mipmaps.
    const requiresMipmaps = [
      NearestMipmapNearestFilter,
      NearestMipmapLinearFilter,
      LinearMipmapNearestFilter,
      LinearMipmapLinearFilter
    ].includes(minFilter)

    if (!generateMipmaps && requiresMipmaps) {
      // Si no hay mipmaps, WebGL caerá en NearestFilter o LinearFilter automáticamente
      // pero para claridad pedagógica, forzamos LinearFilter o NearestFilter en la textura
      texture.minFilter = (minFilter === NearestMipmapNearestFilter || minFilter === NearestMipmapLinearFilter) 
        ? NearestFilter 
        : LinearFilter
    }

    texture.needsUpdate = true
  }

  /**
   * Configura la anisotropía de la textura.
   * La anisotropía reduce el efecto borroso en texturas que se ven en ángulos muy oblicuos
   * (por ejemplo, un suelo alejándose hacia el horizonte).
   */
  updateAnisotropy(texture, value, maxAnisotropy) {
    if (!texture) return
    
    // Limitar el valor al máximo soportado por el hardware de la GPU
    const safeValue = Math.min(value, maxAnisotropy)
    texture.anisotropy = safeValue
    texture.needsUpdate = true
  }
}

export default new TextureManager()
