{ stdenv
, glfw
, cglm
, glslang
, vulkan-headers
, vulkan-loader
, vulkan-validation-layers
, libX11
, libXrandr
, libXi
, libXxf86vm
}:

stdenv.mkDerivation {
  name = "RanaEngine";
  src = ./.;

  buildPhase = ''
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp VulkanTest $out/bin/RanaEngine
  '';

  buildInputs = [
    glfw vulkan-headers cglm
    glslang
    vulkan-headers
    vulkan-loader
    vulkan-validation-layers
    libX11
    libXrandr
    libXi
    libXxf86vm
  ];

  shellHook = ''
    export VK_LAYER_PATH="${vulkan-validation-layers}/share/vulkan/explicit_layer.d";
  '';

  outputs = ["out"];
}

