#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h> 
#include <windows.h>

/*==========================
    Fit Screen
  ========================== */
void center_text(const char *text) {
    // 1. Dapatkan informasi buffer konsol (Screen size)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // Hitung lebar terminal yang tersedia
    int screen_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    
    // 2. Hitung jumlah spasi yang dibutuhkan
    int text_length = strlen(text);
    int padding = (screen_width - text_length) / 2;

    // Pastikan padding tidak negatif
    if (padding < 0) {
        padding = 0; 
    }

    // 3. Cetak spasi, lalu cetak teks
    for (int i = 0; i < padding; i++) {
        printf(" ");
    }
    printf("%s\n", text);
}

/*==========================
    Full Screen Mode
  ========================== */

void maximize_console_window() {
    // 1. Dapatkan handle ke jendela konsol
    HWND hwnd = GetConsoleWindow();

    // 2. Jika handle valid, panggil ShowWindow untuk memaksimalkannya
    if (hwnd != NULL) {
        // SW_MAXIMIZE adalah kode untuk memaksimalkan jendela
        ShowWindow(hwnd, SW_MAXIMIZE); 
    }
}

/* =========================
   COLOR CODES (ANSI)
   ========================= */
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BOLD    "\x1b[1m"

/* =========================
   CONSTANTS & CONFIG
   ========================= */
#define MAX_FOOD_ITEMS 10
#define LEVEL_BEGINNER 1
#define LEVEL_INTERMEDIATE 2
#define LEVEL_EXPERT 3
#define TOLERANCE_BUFFER 150 // Toleransi +/- kalori (Zona Aman)

/* =========================
   DATA STRUCTURES
   ========================= */

typedef struct {
    int id;
    char name[40];
    float calories; // per porsi
} FoodItem;

typedef struct {
    char username[50];
    int userLevel;      
    
    // Data Fisik
    int age;
    char gender;        
    float height;       
    float weight;       
    float initialWeight; 
    float bmi;
    
    // Status Program
    int isProgramActive;
    float bmr;          
    float tdee;         
    float calorieTarget;
    char focus[30];     

    // Tracking Harian
    float caloriesIn;
    float caloriesOut;
} User;

// Database Makanan (Tetap sama)
FoodItem foodDB[MAX_FOOD_ITEMS] = {
    {1, "Nasi Putih (1 piring)", 204},
    {2, "Dada Ayam Rebus (100g)", 165},
    {3, "Ayam Goreng (1 potong)", 260},
    {4, "Telur Rebus (1 butir)", 78},
    {5, "Tempe Goreng (2 ptg)", 150},
    {6, "Sayur Sop (1 mangkuk)", 75},
    {7, "Pisang (1 buah)", 105},
    {8, "Whey Protein (1 scoop)", 120},
    {9, "Mie Instan (1 bungkus)", 380},
    {10, "Kopi Susu Gula Aren", 220}
};

/* =========================
   COACH UTILITIES (NEW)
   ========================= */

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void cleanBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void pressEnterToContinue() {
    printf(ANSI_CYAN "\n[ Tekan ENTER untuk lanjut... ]" ANSI_RESET);
    cleanBuffer();
    getchar();
}

char* getCurrentTime() {
    time_t t;
    time(&t);
    char* timeStr = ctime(&t);
    timeStr[strlen(timeStr)-1] = '\0';
    return timeStr;
}

// Fitur Visualisasi Progress Bar (ASCII) - DITAMBAHKAN DARI V5
void drawProgressBar(float current, float target, char *focus) {
    int width = 30; // Lebar bar dalam karakter
    float percent = current / target;
    if (percent > 1.0) percent = 1.0;
    if (percent < 0) percent = 0;

    int filled = (int)(percent * width);
    int empty = width - filled;

    // Tentukan Warna Bar
    char *barColor = ANSI_BLUE; 
    
    float diff = target - current;
    
    // Logika Warna: Hijau jika dekat target, Kuning jika on progress, Merah jika over
    if (percent < 0.8) {
        barColor = ANSI_BLUE; // Masih jauh (Need Fuel)
    } else if (abs(diff) <= TOLERANCE_BUFFER) {
        barColor = ANSI_GREEN; // ZONA IDEAL
    } else if (current > target + TOLERANCE_BUFFER) {
        barColor = ANSI_RED; // Over
    } else {
        barColor = ANSI_YELLOW; // Mendekati
    }

    // Print Bar
    printf("\n   [");
    printf("%s", barColor);
    for (int i = 0; i < filled; i++) printf("#");
    printf(ANSI_RESET);
    for (int i = 0; i < empty; i++) printf(".");
    printf("] %.0f%%\n", (current/target)*100);

    // Status Teks di bawah Bar
    printf("   Status: ");
    if (abs(diff) <= TOLERANCE_BUFFER) {
        printf(ANSI_BOLD ANSI_GREEN "PERFECT ZONE (On Track) \u2713\n" ANSI_RESET);
    } else if (current > target + TOLERANCE_BUFFER) {
         if (strcmp(focus, "Bulking") == 0) printf(ANSI_YELLOW "Extra Surplus (Good for gains!)\n" ANSI_RESET);
         else printf(ANSI_RED "Over Limit (Hati-hati)\n" ANSI_RESET);
    } else {
        printf(ANSI_BLUE "Fueling Up (Butuh asupan)\n" ANSI_RESET);
    }
}

// Fitur Sapaan Waktu (Personal Touch)
void printGreeting(char *name) {
    time_t now;
    struct tm *local;
    time(&now);
    local = localtime(&now);

    int hour = local->tm_hour;
    char greeting[20];

    if (hour >= 5 && hour < 12) strcpy(greeting, "Selamat Pagi");
    else if (hour >= 12 && hour < 15) strcpy(greeting, "Selamat Siang");
    else if (hour >= 15 && hour < 18) strcpy(greeting, "Selamat Sore");
    else strcpy(greeting, "Selamat Malam");

    printf(ANSI_BOLD ANSI_MAGENTA "%s, %s!" ANSI_RESET " Siap untuk lebih sehat hari ini?\n", greeting, name);
}

// Helper Validasi Input
float getValidFloat(char *prompt, float min, float max) {
    float val;
    int status;
    do {
        printf("%s", prompt);
        status = scanf("%f", &val);
        if (status != 1) {
            printf(ANSI_RED ">> Ups, masukkan angka ya.\n" ANSI_RESET);
            cleanBuffer();
            continue;
        }
        if (val < min || val > max) {
            printf(ANSI_RED ">> Angkanya sepertinya kurang pas (Range: %.0f - %.0f).\n" ANSI_RESET, min, max);
        }
    } while (status != 1 || val < min || val > max);
    return val;
}

int getValidInt(char *prompt, int min, int max) {
    int val;
    int status;
    do {
        printf("%s", prompt);
        status = scanf("%d", &val);
        if (status != 1) {
            printf(ANSI_RED ">> Pilih pakai nomor ya.\n" ANSI_RESET);
            cleanBuffer();
            continue;
        }
        if (val < min || val > max) {
            printf(ANSI_RED ">> Pilihan tidak tersedia.\n" ANSI_RESET);
        }
    } while (status != 1 || val < min || val > max);
    return val;
}

// Navbar "Coach Style" - Colorful
void showHeader(User *u, char *currentScreen) {
    clearScreen();
    printf("\n");
    printf(ANSI_BOLD ANSI_BLUE "========================================\n" ANSI_RESET);
    if (strlen(u->username) > 0) {
        printf(ANSI_BOLD ANSI_CYAN " COACH HEALTHY-W" ANSI_RESET "  |  Halo, " ANSI_YELLOW "%s \n" ANSI_RESET, u->username);
    } else {
        printf(ANSI_BOLD ANSI_CYAN " COACH HEALTHY-W" ANSI_RESET "  |  Setup Mode \n");
    }
    printf(ANSI_BOLD ANSI_BLUE "----------------------------------------\n" ANSI_RESET);
    printf(" Aktivitas: " ANSI_GREEN "%s\n" ANSI_RESET, currentScreen);
    printf(ANSI_BOLD ANSI_BLUE "========================================\n\n" ANSI_RESET);
}

/* =========================
   CORE LOGIC & CALCULATION
   ========================= */

float calculateBMI(float height, float weight) {
    return weight / pow(height / 100, 2);
}

// Kategori dengan bahasa lebih halus
const char* bmiCategory(float bmi) {
    if (bmi < 18.5) return "Berat Badan Kurang";
    if (bmi < 25)   return "Berat Ideal";
    if (bmi < 30)   return "Berat Berlebih";
    return "Obesitas";
}

float calculateBMR(float weight, float height, int age, char gender) {
    float bmr;
    if (gender == 'L' || gender == 'l') {
        bmr = (10 * weight) + (6.25 * height) - (5 * age) + 5;
    } else {
        bmr = (10 * weight) + (6.25 * height) - (5 * age) - 161;
    }
    return bmr;
}

/* =========================
   FILE HANDLING
   ========================= */
// Logika file handling tetap sama agar data aman
void logBMI(User *u) {
    FILE *f = fopen("riwayat_bmi.txt", "a");
    if (f) {
        fprintf(f, "%s | %s | BMI: %.2f (%s)\n", 
                getCurrentTime(), u->username, u->bmi, bmiCategory(u->bmi));
        fclose(f);
        printf(ANSI_GREEN ">> [Coach] Data sudah saya simpan di catatan rahasia kita (riwayat_bmi.txt).\n" ANSI_RESET);
    }
}

void logCalories(User *u) {
    FILE *f = fopen("riwayat_kalori.txt", "a");
    if (f) {
        fprintf(f, "%s | %s | Target: %.0f | Net: %.0f\n", 
                getCurrentTime(), u->username, u->calorieTarget, 
                (u->caloriesIn - u->caloriesOut));
        fclose(f);
        printf(ANSI_GREEN ">> [Coach] Jurnal harianmu berhasil disimpan!\n" ANSI_RESET);
    }
}

void logWeight(User *u) {
    FILE *f = fopen("riwayat_berat.txt", "a");
    if (f) {
        float diff = u->weight - u->initialWeight;
        char sign = (diff >= 0) ? '+' : '-';
        fprintf(f, "%s | %s | Berat: %.1f kg | Progress: %c%.1f kg\n", 
                getCurrentTime(), u->username, u->weight, sign, fabs(diff));
        fclose(f);
        printf(ANSI_GREEN ">> [Coach] Progres tercatat. Konsistensi adalah kunci!\n" ANSI_RESET);
    }
}

void viewHistoryFile(char *filename, char *title) {
    FILE *f = fopen(filename, "r");
    char buffer[255];
    
    printf(ANSI_BOLD "\n--- %s ---\n" ANSI_RESET, title);
    if (f) {
        while (fgets(buffer, 255, f)) {
            printf("%s", buffer);
        }
        fclose(f);
    } else {
        printf(ANSI_YELLOW "Belum ada catatan nih. Yuk mulai tracking!\n" ANSI_RESET);
    }
    printf(ANSI_BOLD "----------------------------------------\n" ANSI_RESET);
}

/* =========================
   FEATURES: FOOD & TRACKING (COACH STYLE)
   ========================= */

void menuFoodDatabase(User *u) {
    showHeader(u, "Catat Makanan");
    
    printf("Kamu habis makan apa yang enak? Jujur aja ya :)\n\n");
    printf(ANSI_BOLD "%-3s | %-25s | %s\n" ANSI_RESET, "No", "Menu", "Kalori");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < MAX_FOOD_ITEMS; i++) {
        printf("%-3d | %-25s | " ANSI_YELLOW "%.0f kkal\n" ANSI_RESET, 
               foodDB[i].id, foodDB[i].name, foodDB[i].calories);
    }
    printf("11  | Input Manual (Custom)     | -\n");
    printf("0   | Gak jadi catat            | -\n");
    
    int choice = getValidInt("\nPilih nomor menunya: ", 0, 11);
    if (choice == 0) return;

    float finalCal = 0;

    if (choice == 11) {
        finalCal = getValidFloat("Berapa kira-kira kalorinya? ", 1, 2000);
        printf(ANSI_GREEN ">> Oke, noted!\n" ANSI_RESET);
    } else {
        printf("Berapa porsi? (misal 1.0 buat sepiring, 0.5 buat setengah): ");
        float porsi;
        scanf("%f", &porsi); // Simple scan for flow
        
        finalCal = foodDB[choice-1].calories * porsi;
        printf(ANSI_GREEN ">> Sip! Menambahkan %s sebanyak %.1f porsi.\n" ANSI_RESET, foodDB[choice-1].name, porsi);
    }

    u->caloriesIn += finalCal;
    printf(ANSI_CYAN ">> Total %.0f kkal ditambahkan ke harianmu.\n" ANSI_RESET, finalCal);
}

void menuExercise(User *u) {
    showHeader(u, "Catat Olahraga");
    printf("Wah, habis gerak nih? Keren! Ngapain aja tadi?\n\n");
    printf("1. Jalan Santai (Healing tipis-tipis)\n");
    printf("2. Lari / Jogging (Bakar serius)\n");
    printf("3. Angkat Beban (Build muscle!)\n");
    printf("4. Bersepeda (Gowes)\n");
    printf("0. Batal\n");
    
    int choice = getValidInt("Pilih aktivitas: ", 0, 4);
    if (choice == 0) return;

    float dur = getValidFloat("Berapa menit durasinya? ", 1, 300);
    float burned = 0;

    switch(choice) {
        case 1: burned = dur * 3.5; break;
        case 2: burned = dur * 10; break;
        case 3: burned = dur * 6; break;
        case 4: burned = dur * 8; break;
    }
    
    u->caloriesOut += burned;
    printf(ANSI_GREEN "\n>> Mantap! Keringatmu hari ini membakar %.0f kkal.\n" ANSI_RESET, burned);
    printf(ANSI_CYAN ">> Jangan lupa minum air putih ya!\n" ANSI_RESET);
}

/* =========================
   FEATURES: INSIGHT (COACH VOICE)
   ========================= */

void showDailyInsight(User *u) {
    float net = u->caloriesIn - u->caloriesOut;
    float remaining = u->calorieTarget - net;
    
    printf(ANSI_BOLD ANSI_MAGENTA "\n[ KATA COACH HARI INI ]\n" ANSI_RESET);
    
    // Logika Coach yang lebih mengalir dan empati
    if (remaining > 0) {
        printf(">> Kamu masih punya sisa " ANSI_GREEN "%.0f kalori" ANSI_RESET ".\n", remaining);
        if (strcmp(u->focus, "Bulking") == 0) {
            printf(">> Ayo makan lagi! Otot butuh bahan bakar untuk tumbuh.\n");
        } else {
            printf(">> Kamu dalam posisi aman. Kalau lapar, snack buah oke banget.\n");
        }
    } else {
        printf(">> Kamu surplus " ANSI_RED "%.0f kalori" ANSI_RESET " dari target.\n", fabs(remaining));
        if (strcmp(u->focus, "Cutting") == 0) {
            printf(">> Nggak apa-apa, besok kita coba lebih disiplin lagi ya.\n");
            printf(">> Atau coba jalan santai 15 menit malam ini untuk bakar dikit?\n");
        } else {
            printf(">> Surplus bagus buat Bulking! Pastikan proteinnya cukup ya.\n");
        }
    }
}

void menuWeightProgress(User *u) {
    showHeader(u, "Progress Berat Badan");
    
    if (u->initialWeight == 0) u->initialWeight = u->weight;
    
    float diff = u->weight - u->initialWeight;

    printf("Mari kita lihat perjalananmu sejauh ini:\n\n");
    printf("Berat Awal     : %.1f kg\n", u->initialWeight);
    printf("Berat Sekarang : " ANSI_BOLD "%.1f kg\n" ANSI_RESET, u->weight);
    
    if (diff < 0) printf("Perubahan      : " ANSI_GREEN "Turun %.1f kg! Wow!\n\n" ANSI_RESET, fabs(diff));
    else if (diff > 0) printf("Perubahan      : " ANSI_YELLOW "Naik %.1f kg.\n\n" ANSI_RESET, diff);
    else printf("Perubahan      : " ANSI_BLUE "Stabil.\n\n" ANSI_RESET);

    printf("Mau update berat badan hari ini?\n");
    printf("1. Ya, aku habis nimbang\n");
    printf("2. Nggak dulu, cuma mau lihat log\n");
    printf("0. Kembali\n");
    
    int ch = getValidInt("Pilih: ", 0, 2);
    if (ch == 1) {
        u->weight = getValidFloat("Berapa beratmu sekarang (kg)? ", 30, 300);
        logWeight(u);
        
        // Update data real-time
        u->bmi = calculateBMI(u->height, u->weight);
        u->bmr = calculateBMR(u->weight, u->height, u->age, u->gender);
        
        printf(ANSI_GREEN "\n>> Data berhasil diupdate. Terus konsisten ya, %s!\n" ANSI_RESET, u->username);
        pressEnterToContinue();
    } else if (ch == 2) {
        viewHistoryFile("riwayat_berat.txt", "LOG PERJALANAN BERAT BADAN");
        pressEnterToContinue();
    }
}

/* =========================
   MENUS & FLOWS
   ========================= */

void menuProTracking(User *u) {
    if (!u->isProgramActive) {
        printf(ANSI_RED "\n>> Eits, kita harus bikin rencana dulu sebelum mulai tracking.\n" ANSI_RESET);
        pressEnterToContinue();
        return;
    }

    int choice;
    do {
        float net = u->caloriesIn - u->caloriesOut;
        
        showHeader(u, "Dashboard Harian");
        
        // --- FITUR BARU: VISUAL PROGRESS BAR & REAL-TIME FEEDBACK ---
        printf(" TARGET: %.0f kkal | FOKUS: %s\n", u->calorieTarget, u->focus);
        drawProgressBar(net, u->calorieTarget, u->focus); // Fungsi ASCII Bar
        
        printf(ANSI_BLUE "\n ------------------------------\n" ANSI_RESET);
        printf(" Makanan Masuk : " ANSI_YELLOW "%6.0f kkal\n" ANSI_RESET, u->caloriesIn);
        printf(" Olahraga      : " ANSI_GREEN "-%5.0f kkal\n" ANSI_RESET, u->caloriesOut);
        printf(" Net Kalori    : " ANSI_BOLD "%6.0f kkal\n" ANSI_RESET, net);
        printf(ANSI_BLUE " ------------------------------\n" ANSI_RESET);

        // Feedback Real-time
        float diff = u->calorieTarget - net;
        printf("\n [ FEEDBACK PROGRESS ]\n");
        if (abs(diff) <= TOLERANCE_BUFFER) {
            printf(ANSI_GREEN " >> Pertahankan ini! Kamu ada di zona sempurna (On Track).\n" ANSI_RESET);
        } else if (diff > TOLERANCE_BUFFER) {
            printf(" >> Masih butuh " ANSI_YELLOW "%.0f kkal" ANSI_RESET " lagi untuk mencapai target.\n", diff);
        } else {
            // Over logic
            if (strcmp(u->focus, "Bulking") == 0) printf(" >> Surplus kalori tercapai (Good for Bulking).\n");
            else printf(" >> Hati-hati, sudah lewat target harian.\n");
        }
        
        printf("\nApa yang mau kita lakukan sekarang?\n");
        printf("[1] Catat Makanan\n");
        printf("[2] Catat Olahraga\n");
        printf("[3] Simpan Jurnal Hari Ini\n");
        printf("[4] (Reset)\n");
        printf("[0] Kembali ke Menu Utama\n");

        choice = getValidInt("Pilih aksimu: ", 0, 4);

        switch(choice) {
            case 1: menuFoodDatabase(u); pressEnterToContinue(); break;
            case 2: menuExercise(u); pressEnterToContinue(); break;
            case 3: logCalories(u); pressEnterToContinue(); break;
            case 4: 
                u->caloriesIn = 0; u->caloriesOut = 0; 
                printf(ANSI_GREEN ">> Semangat pagi! Data hari ini sudah bersih kembali.\n" ANSI_RESET); 
                pressEnterToContinue(); break;
        }
    } while (choice != 0);
}

void setupProgram(User *u) {
    if (u->height == 0 || u->weight == 0) {
        printf(">> Hitung BMI dulu sebelum buat program ya!", ANSI_RED);
        pressEnterToContinue();
        return;
    }
    showHeader(u, "Konsultasi Program");
    printf("Halo " ANSI_YELLOW "%s" ANSI_RESET ", saya bantu buatkan rencana nutrisi ya.\n", u->username);
    printf("Jawab santai saja...\n\n");
    
    if (u->age == 0) {
        printf("Pertama, jenis kelamin kamu? (L/P): "); cleanBuffer(); scanf("%c", &u->gender);
        u->age = getValidInt("Berapa umur kamu sekarang? ", 10, 100);
    }
    
    u->bmr = calculateBMR(u->weight, u->height, u->age, u->gender);

    printf("\nSeberapa sering kamu bergerak tiap minggu?\n");
    printf("1. Jarang (Rebahan/Kerja duduk)\n");
    printf("2. Lumayan (Olahraga 3-5x seminggu)\n");
    printf("3. Aktif Banget (Kerja fisik/Atlet)\n");
    int act = getValidInt("Pilih (1-3): ", 1, 3);
    float mult = (act==1) ? 1.2 : (act==2) ? 1.55 : 1.725;
    u->tdee = u->bmr * mult;

    printf("\nOke, terakhir. Apa target utamamu?\n");
    printf("1. Kurusin Badan (Cutting)\n");
    printf("2. Jaga Berat Badan Aja (Maintenance)\n");
    printf("3. Besarin Badan/Otot (Bulking)\n");
    int g = getValidInt("Pilih targetmu: ", 1, 3);

    if (g == 1) { strcpy(u->focus, "Cutting"); u->calorieTarget = u->tdee - 500; }
    else if (g == 2) { strcpy(u->focus, "Maintenance"); u->calorieTarget = u->tdee; }
    else { strcpy(u->focus, "Bulking"); u->calorieTarget = u->tdee + 400; }

    u->isProgramActive = 1;
    printf(ANSI_GREEN "\n>> Rencana Siap! Target makanmu: %.0f kkal/hari.\n" ANSI_RESET, u->calorieTarget);
    printf(ANSI_CYAN ">> Kita mulai tracking bareng-bareng ya!\n" ANSI_RESET);
    pressEnterToContinue();
}

/* =========================
   MAIN APP CONTROLLER
   ========================= */

void runUserSession(User *u) {
    int choice;
    do {
        showHeader(u, "Menu Utama");
        printGreeting(u->username);
        printf("\n");

        // MENU ADAPTIF 
        
        printf("[1] Cek Kondisi Tubuhku (BMI)\n");
        
        if (u->userLevel >= LEVEL_INTERMEDIATE) {
            printf("[2] Program & Tracking (Harian)\n");
            printf("[3] Lihat Jurnal Kalori\n");
        }

        if (u->userLevel == LEVEL_EXPERT) {
            printf("[4] Progress Berat Badan (Expert)\n");
        }

        printf("[0] Istirahat Dulu (Keluar)\n");
        printf(ANSI_BLUE "----------------------------------------\n" ANSI_RESET);

        choice = getValidInt("Mau kemana kita? ", 0, 4);

        switch(choice) {
            case 1: 
                showHeader(u, "Cek Kondisi Tubuh");
                if(u->height == 0) {
                     printf("Bantu saya kenal kamu dulu ya.\n");
                     u->height = getValidFloat("Tinggi badan (cm): ", 50, 250);
                     u->weight = getValidFloat("Berat badan (kg): ", 30, 300);
                     u->bmi = calculateBMI(u->height, u->weight);
                }
                
                printf("Hasil Analisa:\n");
                printf("BMI Kamu : " ANSI_BOLD "%.2f\n" ANSI_RESET, u->bmi);
                
                char *colorBMI = ANSI_GREEN;
                if (u->bmi < 18.5) colorBMI = ANSI_YELLOW;
                if (u->bmi >= 25) colorBMI = ANSI_RED;

                printf("Status   : %s%s\n\n" ANSI_RESET, colorBMI, bmiCategory(u->bmi));
                
                // Coach Advice
                if (u->bmi >= 25) printf(ANSI_CYAN ">> Saran Coach: Nggak perlu panik. Fokus kurangi gula dan jalan kaki rutin.\n" ANSI_RESET);
                else if (u->bmi < 18.5) printf(ANSI_CYAN ">> Saran Coach: Perbanyak protein dan karbohidrat kompleks ya.\n" ANSI_RESET);
                else printf(ANSI_GREEN ">> Saran Coach: Pertahankan! Pola hidupmu sudah bagus.\n" ANSI_RESET);

                char sv;
                printf("\nSimpan hasil ini? (y/n): ");
                cleanBuffer(); scanf("%c", &sv);
                if(sv=='y'||sv=='Y') logBMI(u);
                
                pressEnterToContinue();
                break;
            
            case 2:
                if (u->userLevel < LEVEL_INTERMEDIATE) {
                    printf(ANSI_YELLOW ">> Ups, fitur ini terbuka di level Menengah. Yuk upgrade pemahamanmu!\n" ANSI_RESET);
                    pressEnterToContinue();
                } else {
                    if (!u->isProgramActive) setupProgram(u);
                    else menuProTracking(u);
                }
                break;
            
            case 3:
                 if (u->userLevel >= LEVEL_INTERMEDIATE) {
                     viewHistoryFile("riwayat_kalori.txt", "JURNAL KALORI KAMU");
                     pressEnterToContinue();
                 }
                 break;

            case 4:
                if (u->userLevel == LEVEL_EXPERT) {
                    menuWeightProgress(u);
                } else {
                    printf(ANSI_YELLOW ">> Fitur khusus Expert user.\n" ANSI_RESET);
                    pressEnterToContinue();
                }
                break;
                
            case 0: printf(ANSI_MAGENTA "\nSampai jumpa, %s! Jangan lupa senyum hari ini.\n" ANSI_RESET, u->username); break;
        }

    } while (choice != 0);
}

int main() {
    srand(time(0)); 
    User user = {0};
    clearScreen();
    printf("\n");
    center_text(ANSI_BOLD ANSI_CYAN "           /$$   /$$                     /$$   /$$     /$$                 /$$      /$$");
    center_text(" | $$  | $$                    | $$  | $$    | $$                | $$  /$ | $$");
    center_text(" | $$  | $$  /$$$$$$   /$$$$$$ | $$ /$$$$$$  | $$$$$$$  /$$   /$$| $$ /$$$| $$");
    center_text(" | $$$$$$$$ /$$__  $$ |____  $$| $$|_  $$_/  | $$__  $$| $$  | $$| $$/$$ $$ $$");
    center_text(" | $$__  $$| $$$$$$$$  /$$$$$$$| $$  | $$    | $$  \\ $$| $$  | $$| $$$$_  $$$$");
    center_text(" | $$  | $$| $$_____/ /$$__  $$| $$  | $$ /$$| $$  | $$| $$  | $$| $$$/ \\  $$$");
    center_text(" | $$  | $$|  $$$$$$$|  $$$$$$$| $$  |  $$$$/| $$  | $$|  $$$$$$$| $$/   \\  $$");
    center_text(" |__/  |__/ \\_______/ \\_______/|__/   \\___/  |__/  |__/ \\____  $$|__/     \\__/");
    center_text("                                                        /$$  | $$             ");
    center_text("                                                       |  $$$$$$/             ");
    center_text("                                                         \\______/              \n");
    center_text("             VIRTUAL COACH EDITION v4.6 (Visual)          \n\n" ANSI_RESET);
    
    printf("Halo! Saya HealthyW, asisten kesehatan pribadimu.\n");
    printf("Boleh tau nama panggilanmu? ");
    scanf(" %[^\n]s", user.username);

    printf("\nSalam kenal, " ANSI_YELLOW "%s!" ANSI_RESET, user.username);
    printf(" Agar saya bisa membantumu dengan tepat, pilih levelmu\n\n");
    
    printf(ANSI_GREEN "1. PEMULA" ANSI_RESET " (Baru mulai, mau cek kondisi aja)\n");
    printf(ANSI_BLUE "2. MENENGAH" ANSI_RESET " (Sudah biasa tracking makanan & olahraga)\n");
    printf(ANSI_RED "3. EXPERT" ANSI_RESET " (Serius tracking berat badan & detail)\n");
    
    user.userLevel = getValidInt("\nPilih level (1-3): ", 1, 3);
    
    if (user.userLevel == LEVEL_EXPERT) {
        printf(ANSI_RED "\n[EXPERT DETECTED] Oke, kita main data lengkap ya.\n" ANSI_RESET);
        user.height = getValidFloat("Tinggi badan (cm): ", 50, 250);
        user.weight = getValidFloat("Berat badan (kg): ", 30, 300);
        user.initialWeight = user.weight; 
        user.bmi = calculateBMI(user.height, user.weight);
    }

    runUserSession(&user);

    return 0;
}