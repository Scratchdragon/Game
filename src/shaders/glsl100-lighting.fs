#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Player and render data
uniform float rotation;
uniform vec2 center;
uniform float scale;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// NOTE: Add here your custom variables

void main() {
    // Texel color fetching from texture sampler
    vec4 texelColor = texture2D(texture0, fragTexCoord)*colDiffuse*fragColor;

    float color = (distance(gl_FragCoord.xy, center)/2000.0);

    // Calculate final fragment color
    gl_FragColor = vec4(texelColor.r - color, texelColor.g - color, texelColor.b - color, texelColor.a);
}