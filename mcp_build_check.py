import subprocess
import sys

# MCP: Build-Status automatisch prüfen und Ergebnis für KI bereitstellen

def run_build():
    try:
        result = subprocess.run([
            'mingw32-make', '-C', 'build'],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False
        )
        output = result.stdout
        if 'error:' in output:
            print('BUILD_FAILED')
            # Zeige die erste relevante Fehlermeldung
            for line in output.splitlines():
                if 'error:' in line:
                    print(line)
                    break
        else:
            print('BUILD_OK')
    except Exception as e:
        print('BUILD_FAILED')
        print(str(e))

if __name__ == '__main__':
    run_build()
