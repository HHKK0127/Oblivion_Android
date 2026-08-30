import zipfile, os, collections

apk = 'app/build/outputs/apk/debug/app-debug.apk'
with zipfile.ZipFile(apk) as z:
    # Categorize by directory
    dir_sizes = collections.defaultdict(lambda: {'size': 0, 'count': 0})
    for info in z.infolist():
        parts = info.filename.split('/')
        if len(parts) > 1:
            dir_name = '/'.join(parts[:2])
        else:
            dir_name = '(root)'
        dir_sizes[dir_name]['size'] += info.file_size
        dir_sizes[dir_name]['count'] += 1
    
    print('=== Top 15 directories by size ===')
    for dir_name, data in sorted(dir_sizes.items(), key=lambda x: -x[1]['size'])[:15]:
        size_mb = data['size'] / 1024 / 1024
        count = data['count']
        print(f'{size_mb:.2f} MB  ({count} files)  {dir_name}')
    
    print()
    print('=== Asset breakdown by extension ===')
    ext_sizes = collections.defaultdict(lambda: {'size': 0, 'count': 0})
    for info in z.infolist():
        ext = os.path.splitext(info.filename)[1] or '(no ext)'
        ext_sizes[ext]['size'] += info.file_size
        ext_sizes[ext]['count'] += 1
    
    for ext, data in sorted(ext_sizes.items(), key=lambda x: -x[1]['size'])[:20]:
        size_mb = data['size'] / 1024 / 1024
        count = data['count']
        print(f'{size_mb:.2f} MB  ({count} files)  {ext}')
