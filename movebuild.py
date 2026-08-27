import os
import shutil
import sys

def main():
    safemode_src = "/home/j4ck/Dev/Prototipo-2.1 (Limpio)/linux/build/zomboid_safemode.so"
    full_src = "/home/j4ck/Dev/Prototipo-2.1 (Limpio) FULL FEATURES/linux/build/zomboid_full.so"
    dest_dir = "/home/j4ck/Dev"
    dest_file = os.path.join(dest_dir, "zomboid_safemode.so")

    print("==================================================")
    print("      PROTOTIPO BUILD DEPLOYER / SELECTOR         ")
    print("==================================================")
    print("1) ESP Only (SafeMode Pasivo Indetectable)")
    print("2) Full Features (Godmode, Item Spawner, Cheats)")

    choice = ""
    if len(sys.argv) > 1:
        arg = sys.argv[1].lower()
        if "safe" in arg or arg == "1":
            choice = "1"
        elif "full" in arg or arg == "2":
            choice = "2"
    
    if not choice:
        try:
            choice = input("\nSelecciona la versión a desplegar [1/2]: ").strip()
        except (EOFError, KeyboardInterrupt):
            choice = "1"

    if choice == "2":
        selected_src = full_src
        mode_name = "Full Features (zomboid_full.so)"
    else:
        selected_src = safemode_src
        mode_name = "ESP Only (zomboid_safemode.so)"

    if not os.path.exists(selected_src):
        print(f"\n❌ Error: No se encontró el binario en: {selected_src}")
        print("Asegúrate de compilar la versión elegida con 'ninja -C linux/build'.")
        sys.exit(1)

    try:
        shutil.copy2(selected_src, dest_file)
        print(f"\n✅ Éxito: Desplegada la versión [{mode_name}] -> {dest_file}")
        print("Lanza el juego normalmente desde Steam (con ENABLE_SAFEMODE=1 %command%).")
    except Exception as e:
        print(f"\n❌ Error al desplegar binario: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
