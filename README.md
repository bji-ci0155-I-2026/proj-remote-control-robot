# Prototipo de Robot Móvil Teleoperado con Conducción Asistida

Universidad de Costa Rica - Sistemas Empotrados de Tiempo Real - CI-0155

**Profesor**: Prof. Ariel Mora Jiménez

**Integrantes:**
- Isabella Rodríguez Sánchez (C26701)  
- Esteban Isaac Baires Cerdas (C10844)  
- Jorge Ricardo Díaz Sagot (C12565)

---

## Índice de Documentos del Proyecto

El desarrollo y planeamiento del robot móvil se encuentra organizado en las siguientes secciones detalladas:

- **[Propuesta Inicial (Teórica)](./propuesta-inicial.md):** Contiene el planeamiento teórico original del sistema empotrado, incluyendo la estimación de componentes, tecnologías y diagramas de arquitectura de hardware y software planteados al inicio del proyecto.
- **[Pruebas de Concepto y Viabilidad (Prototipo Real)](./pruebas-de-concepto-y-viabilidad.md):** Contiene la documentación técnica del prototipo físico final, detallando los componentes realmente utilizados y descartados, los resultados de las pruebas experimentales y los cambios de ingeniería aplicados para asegurar la viabilidad.

---

## Descripción del sistema empotrado

El proyecto consiste en el diseño y desarrollo de un prototipo de robot móvil de dos ruedas adaptado para superficies planas, enfocado en la conducción asistida. El desarrollo se dividirá en dos etapas iterativas:

- **Producto Mínimo Viable (MVP):** Un sistema de teleoperación manual donde el robot será manejado a distancia a través de una interfaz en un dispositivo móvil, utilizando el protocolo de comunicación inalámbrica estándar integrado en la placa.  
- **Extensión del Sistema (Asistencia Inteligente):** Como mejora al MVP, el robot integrará un modelo de asistencia y fusión de sensores lógicos ejecutado de forma local. Analizando el entorno en tiempo real mediante sensores ultrasónicos de proximidad, el sistema emite alertas físicas sonoras preventivas ante la cercanía de obstáculos y ejecuta una función de frenado automático de emergencia que anula la tracción para evitar colisiones físicas de forma directa en el hardware de control.

---

## Público meta

**Público meta principal**

**Investigación y educación académica**
Es el sector de investigación y educación académica en robótica, enfocado específicamente en el estudio y desarrollo de dispositivos móviles con recursos limitados; con ruedas que requieran capacidades de navegación asistida procesadas localmente por medio de algoritmos eficientes en tiempo real, que utilicen múltiples sensores para mejorar su precisión en decisiones para evadir riesgos.

**Casos de uso secundarios**

**Logística y manufactura inteligente**  
El diseño conceptual es de interés para empresas de logística, almacenamiento y manufactura inteligente. Estas empresas pueden utilizar el prototipo como un modelo a escala para evaluar la viabilidad de integrar asistencia inteligente y alertas preventivas en la teleoperación manual de maquinaria pesada, buscando mejorar la seguridad laboral y reducir la tasa de accidentes sin depender de una infraestructura de procesamiento en la nube.

**Aplicación en sistemas críticos y automoción**  
También puede ser de importancia para el sector de todo tipo de automóvil asistido de pequeña y gran escala críticos, al proveer un ejemplo de cómo se pueden utilizar los recursos de la mejor manera para mantener el procesamiento local, el cual hace más rápidas las decisiones, lo que ahorraría segundos o milisegundos valiosos para sistemas críticos.
