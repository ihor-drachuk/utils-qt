import QtQuick 2.15
import UtilsQt 1.0

ShaderEffect {
    id: root

    // Public properties
    property real intensity: 0.05    // Noise strength (0.0 - 1.0)
    property real grainScale: 400.0  // UV multiplier for noise size (higher = finer grain)
    property real seed: 0.0          // Seed for noise pattern variation

    Component.onCompleted: {
        if (!internal.isQt6) {
            console.warn("GrainEffect: Requires Qt6. Displaying pink fallback.");
        }
    }

    // Qt6: use precompiled .qsb shader
    // Qt5/fallback: inline shader that fills with pink to indicate unsupported
    fragmentShader: internal.isQt6 ?
        "qrc:/UtilsQt/UtilsQt/shaders/grain.frag.qsb" :
        internal.fallbackShader

    QtObject {
        id: internal

        readonly property bool isQt6: QmlUtils.qtVersionMajor >= 6

        // Fallback shader for Qt5 - fills with semi-transparent pink
        readonly property string fallbackShader: "
            varying highp vec2 qt_TexCoord0;
            uniform lowp float qt_Opacity;
            uniform sampler2D source;

            void main() {
                vec4 tex = texture2D(source, qt_TexCoord0);
                // Mix original with pink to show fallback state
                vec3 pink = vec3(1.0, 0.412, 0.706); // #FF69B4
                gl_FragColor = vec4(mix(tex.rgb, pink, 0.5), tex.a) * qt_Opacity;
            }
        "
    }
}
