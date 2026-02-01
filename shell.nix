{ pkgs ? import <nixpkgs> {} }:

let
  myBuildInputs = with pkgs; [
    # Graphics & Windowing
    glm
    sdl3
    sdl3-image
    glew
    
    # Vulkan Dependencies
    vulkan-volk
    vulkan-loader
    vulkan-headers
    vulkan-validation-layers
    
    # System Libs
    libGL
    libGLU
    xorg.libX11
    xorg.libXext
    xorg.libXrandr
    libxkbcommon

    nlohmann_json
    stb
  ];
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    lld
    clang
    gcc-unwrapped
    cmake
    pkg-config
    tinygltf
  ];

  buildInputs = myBuildInputs;

  shellHook = ''
    # 1. Vulkan: Allow the app to find validation layers
    export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
    
    # 2. Vulkan: Tell the loader where the NVIDIA driver is (NixOS specific)
    export VK_ICD_FILENAMES=/run/opengl-driver/share/vulkan/icd.d/nvidia_icd.json

    # 3. OpenGL/GLEW: Force SDL to use X11 (XWayland)
    # GLEW breaks on native Wayland because it looks for GLX symbols.
    export SDL_VIDEO_DRIVER=x11 

    # 4. Library Paths
    export LD_LIBRARY_PATH=${pkgs.lib.makeLibraryPath myBuildInputs}:/run/opengl-driver/lib:/run/opengl-driver-32/lib:$LD_LIBRARY_PATH
    
    # 5. Build Paths
    export CPATH=${pkgs.lib.makeSearchPathOutput "dev" "include" myBuildInputs}:$CPATH
    export LIBRARY_PATH=${pkgs.lib.makeLibraryPath myBuildInputs}:$LIBRARY_PATH
    
    export NIX_ENFORCE_NO_NATIVE=0
    echo "Vulkan (with Layers) + GLEW (X11 Mode) shell ready!"
    # Add STB include path
    export CPATH=${pkgs.stb}/include/stb:${pkgs.lib.makeSearchPathOutput "dev" "include" myBuildInputs}:$CPATH
  '';
}