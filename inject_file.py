import sys
import os
import struct

def inject(img_path, file_path, target_name):
    with open(img_path, "r+b") as f:
        # Read BPB
        f.seek(11)
        bytes_per_sector = struct.unpack("<H", f.read(2))[0]
        sectors_per_cluster = struct.unpack("B", f.read(1))[0]
        reserved_sectors = struct.unpack("<H", f.read(2))[0]
        fats = struct.unpack("B", f.read(1))[0]
        root_entries = struct.unpack("<H", f.read(2))[0]
        
        f.seek(22)
        sectors_per_fat = struct.unpack("<H", f.read(2))[0]
        
        fat_start = reserved_sectors * bytes_per_sector
        root_start = fat_start + (fats * sectors_per_fat * bytes_per_sector)
        root_size = root_entries * 32
        data_start = root_start + root_size
        
        # Read file data
        with open(file_path, "rb") as src:
            data = src.read()
        
        size = len(data)
        clusters_needed = (size + (sectors_per_cluster * bytes_per_sector) - 1) // (sectors_per_cluster * bytes_per_sector)
        
        # Find free clusters and directory entry
        # For simplicity, we assume the first file and fresh image
        # Find first empty root entry
        f.seek(root_start)
        entry_idx = -1
        for i in range(root_entries):
            entry = f.read(32)
            if entry[0] == 0x00 or entry[0] == 0xE5:
                entry_idx = i
                break
        
        if entry_idx == -1:
            print("No free root entries")
            sys.exit(1)
            
        # Find free clusters in FAT (skipping reserved 0 and 1)
        f.seek(fat_start + 4) # Skip cluster 0 and 1
        free_clusters = []
        for i in range(2, 65536):
            val = struct.unpack("<H", f.read(2))[0]
            if val == 0:
                free_clusters.append(i)
                if len(free_clusters) == clusters_needed:
                    break
        
        if len(free_clusters) < clusters_needed:
            print("Not enough free clusters")
            sys.exit(1)
            
        # Update FATs
        for fat_idx in range(fats):
            current_fat_start = fat_start + (fat_idx * sectors_per_fat * bytes_per_sector)
            for i in range(clusters_needed):
                f.seek(current_fat_start + free_clusters[i] * 2)
                if i == clusters_needed - 1:
                    f.write(struct.pack("<H", 0xFFFF)) # EOF
                else:
                    f.write(struct.pack("<H", free_clusters[i+1]))
        
        # Write data
        for i in range(clusters_needed):
            cluster = free_clusters[i]
            cluster_offset = data_start + (cluster - 2) * sectors_per_cluster * bytes_per_sector
            f.seek(cluster_offset)
            start = i * sectors_per_cluster * bytes_per_sector
            end = min(start + sectors_per_cluster * bytes_per_sector, size)
            f.write(data[start:end])
            
        # Write directory entry
        f.seek(root_start + entry_idx * 32)
        # Name (8.3)
        name_parts = target_name.upper().split('.')
        base = name_parts[0].ljust(8)[:8]
        ext = (name_parts[1] if len(name_parts) > 1 else "").ljust(3)[:3]
        f.write(base.encode('ascii'))
        f.write(ext.encode('ascii'))
        f.write(struct.pack("B", 0x20)) # Attr: Archive
        f.write(b'\x00' * 10) # Reserved
        f.write(struct.pack("<H", 0)) # Time
        f.write(struct.pack("<H", 0)) # Date
        f.write(struct.pack("<H", free_clusters[0])) # First cluster
        f.write(struct.pack("<I", size)) # Size

if __name__ == "__main__":
    inject(sys.argv[1], sys.argv[2], sys.argv[3])
