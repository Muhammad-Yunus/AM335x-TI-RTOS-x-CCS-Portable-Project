### CONTEXT ####
- Ini adalah project CCS + TI-RTOS (SYS/BIOS) PDK hasil copy dari PDK example MCSPI_SlaveMode_MasterExample_bbbAM335x_armExampleProject lalu copy project ke workspace
- Target hardware: AM3352 (board mirip Beaglebone Black) dengan Ethernet (ICSS/EMAC)
- Project ini berjalan sempurna sesuai fungsinya (SPI Slave Mode - master side)

### OBJECTIVE ####
Optimalkan project ini agar strukturnya bersih, maintainable, dan siap dikembangkan.
Lakukan tahap-tahap berikut SATU PER SATU (jangan skip):

#### TAHAP 1: Hapus Linked Resources dari .cproject
- Buka file .cproject, cari semua <linkedResource yang mengarah ke file .c eksternal
- cek di project setting (.project atau di .cproject) ada disana, gak mungkin gak ada, aku cek via project setting, tandanya file punya panah.
- Catat daftar file yang di-link
- Hapus semua <linkedResource> entries dari .cproject 

#### TAHAP 2: Copy Linked Files ke Local Project
- Copy semua file .c yang tadi di-link ke root folder project ini, 
- Pastikan file yang dicopy sudah ada (jika belum, buat file kosong sebagai placeholder)
- Setelah dicopy, tambahkan file-file lokal ini ke project setting
- ingat jangan copy file diluar daftar linked resource di point 1

#### TAHAP 3: Ganti Path Relative → Absolute via COM_TI_PDK_INSTALL_DIR
- Buka file .cproject, cari semua path yang mengandung reference ke original project root
 (misalnya path yang mengandung "pdk_am335x_1_0_17" dengan path relatif)
- Ganti SEMUA path include (-I) menjadi absolute path mengunakan variabel COM_TI_PDK_INSTALL_DIR
  Contoh: COM_TI_PDK_INSTALL_DIR = C:/ti/pdk_am335x_1_0_17
  Path include: COM_TI_PDK_INSTALL_DIR/packages, COM_TI_PDK_INSTALL_DIR/ti/drv/emac, dll.
- Pastikan semua path include di subsystem GNU Compiler dan linker menggunakan absolute path

#### TAHAP 4: Slim Down main.c
- Hapus semua komentar boilerplate yang tidak perlu (tanggal, author, versi detail)
- Hapus loop infinite yang berulang-ulang (pertahankan satu loop utama yang jelas)
- Hapus dead code / fungsi yang tidak dipanggil (cek dengan grep), misal code untuk SOC diluar AM335x, atau code related DMA (karena disini tidak pakai DMA)
- Pertahankan hanya kode inti: init board → init SPI → spi tx '0xAF' → loop
- Pastikan kode tetap berfungsi: init SPI + SPI tx

#### TAHAP 5: Hapus File Tidak Terpakai
- Cek apakah ada file profiling.c / profiling.h / macros.ini_initial di project ini
- Jika ada, cek apakah file-file ini dipanggil dari main.c atau file lain (grep pattern: profile_)
- Jika TIDAK dipakai, HAPUS file-file tersebut
- Cek juga file .o/.dep di folder src/ — ini generated files, JANGAN dihapus tapi abaikan

#### TAHAP 6: Verifikasi Build
- Jalankan: & "C:\ti\ccs1281\ccs\utils\bin\gmake" -k -j 12 all -O 2>&1
  dari folder Debug/ project ini
- Jika ada error, FIX sampai build clean sukses tanpa error
- Pastikan output .out file berhasil di-generate

### RULES ##
- JANGAN MODIFIKASI file apa pun di C:\ti\ (PDK, BIOS, CCS utils, GCC toolchain)
- JANGAN MODIFIKASI file di luar folder project ini
- Lakukan setiap tahap SATU PER SATU, konfirmasi setiap tahap selesai sebelum lanjut
- Jika menemukan file yang curiga tidak terpakai, cek dulu dengan grep sebelum menghapus
- Setelah build sukses, tampilkan ringkasan perubahan yang dilakukan

### PATH REFERENCE ##
COM_TI_PDK_INSTALL_DIR = C:/ti/pdk_am335x_1_0_17
COM_TI_CCS_INSTALL_DIR = C:/ti/ccs1281/ccs
COM_TI_GC_INSTALL_DIR = C:/ti/gcc-arm-none-eabi-7-2018-q2-update 