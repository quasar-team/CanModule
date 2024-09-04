import os
import urllib.request


def download_file(url, save_path):
    # Create directories if they don't exist
    os.makedirs(os.path.dirname(save_path), exist_ok=True)

    try:
        # Download the file
        urllib.request.urlretrieve(url, save_path)
        print(f"File downloaded successfully and saved to {save_path}")
    except Exception as e:
        print(f"Failed to download the file. Error: {e}")


if __name__ == "__main__":
    url = "https://raw.githubusercontent.com/MiguensOne/Github-Gitlab-Sync/main/gitlab-bridge.yml"
    save_path = ".github/workflows/gitlab-bridge.yml"

    download_file(url, save_path)
