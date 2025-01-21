{
  description = "Rana flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    kein.url = "github:Poly2it/kein";
  };

  outputs = { nixpkgs, kein, ... }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
    inherit (pkgs) lib;

    langs = kein.langs;
    toolchain = (langs.c.backends.gcc.makeToolchain ./. {
      ccOptions = {
        features = {
          lto = true;
          fatLtoObjects = true;
        };
        optimizations = {
          level = 0;
        };
        machine = {
          arch = "x86-64";
        };
        debugging = {
          gdb = true;
        };
        standard = "gnu2x";
      };
    });

    rana = (
      langs.c.makeExecutable
      toolchain
      { links = ["glfw" "vulkan" "dl" "pthread" "X11" "Xxf86vm" "Xrandr" "Xi" "m"]; }
      [
        (langs.c.makeTranslationUnit toolchain { includes = with pkgs; [
          glfw
          cglm
          libGL
          glslang
          vulkan-headers
          vulkan-loader
          vulkan-validation-layers
        ] ++ (with pkgs.xorg; [
          libX11
          libXrandr
          libXi
          libXxf86vm
        ]); } ./main.c)
      ]
      "rana"
    );
  in rec {
    packages.x86_64-linux.default = rana.derivation;
    packages.x86_64-linux.propagateCompileCommands = kein.langs.c.makeCompileCommandsPropagator toolchain rana.translationUnits;
    packages.x86_64-linux.debugger = (pkgs.writeShellScriptBin "debugger" "${pkgs.gdbgui |> lib.getExe} ${packages.x86_64-linux.default |> lib.getExe} $@");
    devShells.x86_64-linux.default = pkgs.mkShell {
      shellHook = ''
        export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
      '';
    };
  };
}

