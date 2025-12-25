Import("env")
import os
import subprocess

def webrelease_callback(*args, **kwargs):
    """Deploy webflasher files to h-i-r.net server"""
    print("=" * 60)
    print("Deploying webflasher to h-i-r.net...")
    print("=" * 60)

    # Get the project directory
    project_dir = env.get("PROJECT_DIR")
    webflasher_dir = os.path.join(project_dir, "webflasher")

    # Check if webflasher directory exists
    if not os.path.exists(webflasher_dir):
        print(f"ERROR: webflasher directory not found at {webflasher_dir}")
        return

    # Change to webflasher directory and execute scp
    try:
        print(f"Changing to directory: {webflasher_dir}")
        print("Executing: scp -r * h-i-r.net:/var/www/docs/nautilus.h-i-r.net/")

        result = subprocess.run(
            "scp -r * h-i-r.net:/var/www/docs/nautilus.h-i-r.net/",
            cwd=webflasher_dir,
            shell=True,
            capture_output=True,
            text=True
        )

        if result.returncode == 0:
            print("=" * 60)
            print("Successfully deployed to h-i-r.net!")
            print("=" * 60)
        else:
            print("=" * 60)
            print(f"ERROR: Deployment failed with return code {result.returncode}")
            if result.stderr:
                print(f"Error output: {result.stderr}")
            print("=" * 60)

    except Exception as e:
        print("=" * 60)
        print(f"ERROR: Exception during deployment: {e}")
        print("=" * 60)

# Register the custom target
env.AlwaysBuild(env.Alias("webrelease", None, webrelease_callback))
