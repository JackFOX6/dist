# 1. Detener procesos fantasmas que bloqueen carpetas 
Stop-Process -Name "cmake" -ErrorAction SilentlyContinue 
Stop-Process -Name "ninja" -ErrorAction SilentlyContinue 

# 2. Purgar entornos corruptos previos de forma segura 
if (Test-Path "cmake-3.29.3-windows-x86_64") { Remove-Item -Recurse -Force "cmake-3.29.3-windows-x86_64" -ErrorAction SilentlyContinue } 
if (Test-Path "build") { Remove-Item -Recurse -Force "build" -ErrorAction SilentlyContinue } 

# 3. Recrear directorio de compilación limpio 
New-Item -ItemType Directory -Path "build" -Force 

# 4. Extracción limpia y forzada del ZIP original 
Expand-Archive -Path "cmake.zip" -DestinationPath "." -Force 

# 5. Resolver rutas absolutas para blindar el PATH contra truncamientos 
$RootPath = Convert-Path . 
$env:PATH = "$RootPath\w64devkit\bin;$RootPath\cmake-3.29.3-windows-x86_64\bin;" + $env:PATH 

# 6. Fase de Generación y Compilación nativa 
cd build 
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_CXX_COMPILER=g++ 
ninja 