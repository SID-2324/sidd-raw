import os

def rename_files(folder_path, base_name="spear"):
    # Added '.txt' directly to your valid extensions tuple
    valid_extensions = ('.jpg', '.jpeg', '.png', '.bmp', '.webp', '.txt')
    
    count = 1
    
    try:
        files = sorted(os.listdir(folder_path))
        
        for filename in files:
            # Check if the file is an image or a txt file
            if filename.lower().endswith(valid_extensions):
                # Get the file extension (e.g., .jpg or .txt)
                ext = os.path.splitext(filename)[1]
                
                # Construct the new filename
                new_name = f"{base_name}_{count}{ext}"
                
                source = os.path.join(folder_path, filename)
                destination = os.path.join(folder_path, new_name)
                
                os.rename(source, destination)
                print(f"Renamed: {filename} -> {new_name}")
                
                count += 1
                
        print(f"\nSuccess! Renamed {count - 1} files.")
        
    except FileNotFoundError:
        print("Error: The specified folder path does not exist.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Target path set to your train folder
target_folder = r"C:\Users\Riyan\OneDrive\Desktop\spearhead" 

rename_files(target_folder, "spear")