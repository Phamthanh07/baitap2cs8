/*
 * STREAMFLOW SaaS - Batch Plan Audit Engine
 * Ra soat & ha cap tai khoan tu dong cuoi ky billing.
 * Chuan C11. Bien dich: gcc -std=c11 -Wall -o main main.c
 *
 * Pham vi: mang cau truc (Array of Structs), if/else, vong lap - toan bo trong main().
 * Khong khai bao ham tu dinh nghia nao khac ngoai int main().
 *
 * Danh muc goi:  1 Free     0 VND     toi da 1 thiet bi
 *                2 Personal 120,000   toi da 1 thiet bi
 *                3 Family   250,000   toi da 5 thiet bi
 * Quy tac 1: days_overdue >= 3  -> ha cap ve Free (plan_type = 1), phi = 0
 * Quy tac 2: (sau QT1) Personal va active_devices > 1 -> phu thu 30,000 / thiet bi vuot
 */
#include <stdio.h>
#include <string.h>

struct UserAccount {
    int       user_id;
    char      username[31];
    int       original_plan;    /* goi truoc khi ra soat (de bao cao ha cap) */
    int       plan_type;        /* goi sau khi ra soat */
    int       days_overdue;
    int       active_devices;
    long long base_fee;         /* phi goi sau ra soat */
    long long penalty_fee;      /* phi phat phu thu thiet bi */
    long long total_fee;        /* phi dich vu thuc thu = base + penalty */
    int       is_downgraded;    /* 1: bi ha cap do no cuoc */
};

int main(void) {
    /* ---------------- Hang so nghiep vu ---------------- */
    const int       MAX_TAI_KHOAN     = 100;
    const int       NGAY_NO_HA_CAP    = 3;
    const long long PHI_FREE          = 0;
    const long long PHI_PERSONAL      = 120000;
    const long long PHI_FAMILY        = 250000;
    const int       TB_TOI_DA_FREE    = 1;
    const int       TB_TOI_DA_PERSONAL = 1;
    const int       TB_TOI_DA_FAMILY  = 5;
    const long long PHI_PHAT_MOI_TB   = 30000;

    struct UserAccount users[100];
    int  n, i, j, nhapOK, hopLe, trungMa;
    int  soHaCap = 0, soPhatThietBi = 0, soCanhBaoKhac = 0;
    long long tongDoanhThu = 0, tongTienPhat = 0;

    /* Bien dinh dang tien co dau phay hang nghin */
    long long soTien;
    char chuoiSo[32], chuoiTien[40];
    int  doDai, k;

    printf("================ STREAMFLOW - BATCH PLAN AUDIT ENGINE ================\n");
    printf("Goi: 1 = Free (0 VND, 1 TB) | 2 = Personal (120,000 VND, 1 TB) | 3 = Family (250,000 VND, 5 TB)\n");

    /* ======================= BUOC 1: NHAP N ======================= */
    do {
        printf("\nNhap so luong tai khoan N (1 - %d): ", MAX_TAI_KHOAN);
        nhapOK = scanf("%d", &n);
        while (getchar() != '\n');
        hopLe = (nhapOK == 1 && n >= 1 && n <= MAX_TAI_KHOAN);
        if (!hopLe) {
            printf("LOI: N phai la so nguyen trong khoang [1, %d]. Vui long nhap lai!\n",
                   MAX_TAI_KHOAN);
        }
    } while (!hopLe);

    /* ======================= BUOC 2: NHAP DANH SACH ======================= */
    for (i = 0; i < n; i++) {
        printf("\n--- Tai khoan %d/%d ---\n", i + 1, n);

        /* user_id: so nguyen duong, khong trung */
        do {
            printf("Ma tai khoan (user_id > 0): ");
            nhapOK = scanf("%d", &users[i].user_id);
            while (getchar() != '\n');
            hopLe = (nhapOK == 1 && users[i].user_id > 0);
            trungMa = 0;
            if (hopLe) {
                for (j = 0; j < i; j++) {
                    if (users[j].user_id == users[i].user_id) {
                        trungMa = 1;
                        break;
                    }
                }
            }
            if (!hopLe) {
                printf("LOI: Ma tai khoan phai la so nguyen duong. Vui long nhap lai!\n");
            } else if (trungMa) {
                printf("LOI: Ma tai khoan %d da ton tai. Vui long nhap ma khac!\n",
                       users[i].user_id);
                hopLe = 0;
            }
        } while (!hopLe);

        /* username: chuoi khong khoang trang, toi da 30 ky tu */
        printf("Ten tai khoan (khong khoang trang, toi da 30 ky tu): ");
        if (scanf("%30s", users[i].username) != 1) {
            strcpy(users[i].username, "(trong)");
        }
        while (getchar() != '\n');

        /* plan_type: [1, 3] */
        do {
            printf("Loai goi hien tai (1: Free, 2: Personal, 3: Family): ");
            nhapOK = scanf("%d", &users[i].plan_type);
            while (getchar() != '\n');
            hopLe = (nhapOK == 1 && users[i].plan_type >= 1 && users[i].plan_type <= 3);
            if (!hopLe) {
                printf("LOI: Loai goi phai thuoc khoang [1, 3]. Vui long nhap lai!\n");
            }
        } while (!hopLe);

        /* days_overdue: >= 0 */
        do {
            printf("So ngay no cuoc (>= 0): ");
            nhapOK = scanf("%d", &users[i].days_overdue);
            while (getchar() != '\n');
            hopLe = (nhapOK == 1 && users[i].days_overdue >= 0);
            if (!hopLe) {
                printf("LOI: So ngay no cuoc phai la so nguyen khong am. Vui long nhap lai!\n");
            }
        } while (!hopLe);

        /* active_devices: >= 0 */
        do {
            printf("So thiet bi dang ket noi (>= 0): ");
            nhapOK = scanf("%d", &users[i].active_devices);
            while (getchar() != '\n');
            hopLe = (nhapOK == 1 && users[i].active_devices >= 0);
            if (!hopLe) {
                printf("LOI: So thiet bi phai la so nguyen khong am. Vui long nhap lai!\n");
            }
        } while (!hopLe);
    }

    /* ======================= BUOC 3: RA SOAT & XU LY ======================= */
    for (i = 0; i < n; i++) {
        users[i].original_plan = users[i].plan_type;
        users[i].is_downgraded = 0;
        users[i].penalty_fee = 0;

        /* Quy tac 1: ha cap do no cuoc (ap dung TRUOC) */
        if (users[i].days_overdue >= NGAY_NO_HA_CAP) {
            if (users[i].plan_type != 1) {
                users[i].is_downgraded = 1;     /* chi dem la ha cap neu dang o goi tra phi */
                soHaCap++;
            }
            users[i].plan_type = 1;
        }

        /* Tinh phi goi theo plan_type SAU khi da ap dung Quy tac 1 */
        if (users[i].plan_type == 2) {
            users[i].base_fee = PHI_PERSONAL;
        } else if (users[i].plan_type == 3) {
            users[i].base_fee = PHI_FAMILY;
        } else {
            users[i].base_fee = PHI_FREE;
        }

        /* Quy tac 2: phat vi pham thiet bi - CHI ap dung cho goi Personal sau QT1 */
        if (users[i].plan_type == 2 && users[i].active_devices > TB_TOI_DA_PERSONAL) {
            users[i].penalty_fee = (long long)(users[i].active_devices - TB_TOI_DA_PERSONAL)
                                   * PHI_PHAT_MOI_TB;
            soPhatThietBi++;
            tongTienPhat += users[i].penalty_fee;
        }

        users[i].total_fee = users[i].base_fee + users[i].penalty_fee;
        tongDoanhThu += users[i].total_fee;
    }

    /* ======================= BUOC 4: XUAT BAO CAO ======================= */
    printf("\n=========================== BAO CAO RA SOAT TAI KHOAN ===========================\n");
    printf("%-8s | %-18s | %-12s | %-11s | %-10s | %20s\n",
           "MA TK", "TEN TK", "MA GOI", "SO THIET BI", "SO NGAY NO", "PHI THUC THU (VND)");
    printf("---------------------------------------------------------------------------------\n");

    for (i = 0; i < n; i++) {
        soTien = users[i].total_fee;
        sprintf(chuoiSo, "%lld", soTien);
        doDai = (int)strlen(chuoiSo);
        k = 0;
        for (j = 0; j < doDai; j++) {
            chuoiTien[k++] = chuoiSo[j];
            if ((doDai - j - 1) % 3 == 0 && j != doDai - 1) {
                chuoiTien[k++] = ',';
            }
        }
        chuoiTien[k] = '\0';

        printf("%-8d | %-18.18s | %d %-10s | %-11d | %-10d | %20s\n",
               users[i].user_id, users[i].username, users[i].plan_type,
               users[i].plan_type == 1 ? "(Free)" : (users[i].plan_type == 2 ? "(Personal)" : "(Family)"),
               users[i].active_devices, users[i].days_overdue, chuoiTien);
    }
    printf("---------------------------------------------------------------------------------\n");

    /* Chi tiet xu ly / canh bao */
    printf("\nCHI TIET XU LY:\n");
    for (i = 0; i < n; i++) {
        if (users[i].is_downgraded) {
            printf("  [HA CAP]  TK %d (%s): no cuoc %d ngay -> %s chuyen ve Free, phi 0 VND.\n",
                   users[i].user_id, users[i].username, users[i].days_overdue,
                   users[i].original_plan == 2 ? "Personal" : "Family");
        }
        if (users[i].penalty_fee > 0) {
            printf("  [PHU THU] TK %d (%s): Personal dung %d thiet bi (vuot %d) -> phat %lld VND.\n",
                   users[i].user_id, users[i].username, users[i].active_devices,
                   users[i].active_devices - TB_TOI_DA_PERSONAL, users[i].penalty_fee);
        }
        /* Canh bao vuot gioi han thiet bi o cac goi khong co quy tac phat (khong tinh tien) */
        if (users[i].plan_type == 1 && users[i].active_devices > TB_TOI_DA_FREE) {
            printf("  [CANH BAO] TK %d (%s): goi Free dang co %d thiet bi (toi da %d) - can gioi han phat.\n",
                   users[i].user_id, users[i].username, users[i].active_devices, TB_TOI_DA_FREE);
            soCanhBaoKhac++;
        }
        if (users[i].plan_type == 3 && users[i].active_devices > TB_TOI_DA_FAMILY) {
            printf("  [CANH BAO] TK %d (%s): goi Family dang co %d thiet bi (toi da %d).\n",
                   users[i].user_id, users[i].username, users[i].active_devices, TB_TOI_DA_FAMILY);
            soCanhBaoKhac++;
        }
    }
    if (soHaCap == 0 && soPhatThietBi == 0 && soCanhBaoKhac == 0) {
        printf("  Khong co tai khoan vi pham.\n");
    }

    /* Thong ke */
    printf("\n============================== THONG KE KY BILLING ==============================\n");
    printf("Tong so tai khoan ra soat        : %d\n", n);
    printf("So tai khoan bi ha cap ve Free   : %d\n", soHaCap);
    printf("So tai khoan bi phat thiet bi    : %d (tong phat %lld VND)\n",
           soPhatThietBi, tongTienPhat);

    soTien = tongDoanhThu;
    sprintf(chuoiSo, "%lld", soTien);
    doDai = (int)strlen(chuoiSo);
    k = 0;
    for (j = 0; j < doDai; j++) {
        chuoiTien[k++] = chuoiSo[j];
        if ((doDai - j - 1) % 3 == 0 && j != doDai - 1) {
            chuoiTien[k++] = ',';
        }
    }
    chuoiTien[k] = '\0';
    printf("TONG DOANH THU THUC TE           : %s VND\n", chuoiTien);
    printf("=================================================================================\n");

    return 0;
}
