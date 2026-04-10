# 🧭 MNG Project Onboarding Guide

Welcome to the **Measurement Network Gateway (MNG)** GitLab repository!
This guide explains how to get started, clone the project, and work safely on your assigned branch.

---

## 👥 Team Collaboration Model

* Each team member has their **own personal branch** in this repository.
* The **`main`** branch is protected — only the maintainer (**Saeed Omidvari**) can merge changes into it.
* Everyone commits and pushes **only** to their assigned branch.
* When your work is complete, create a **Merge Request (MR)** to propose merging your branch into `main`.

---

## 🪄 Getting Started (For All Team Members)

### 1️⃣ Accept Your Invitation

After Saeed adds you to the GitLab project, you’ll receive an email.
Click the link to accept the invitation.

---

### 2️⃣ Clone the Repository

Open your terminal and run:

```bash
git clone https://gitlab.com/ESD_MNG_WS25/mng.git
cd mng
```

---

### 3️⃣ Switch to Your Personal Branch

You’ve each been assigned a branch. Run:

```bash
git checkout your-branch-name
```

#### Example:

```bash
git checkout alin-raj
```

Check your current branch anytime:

```bash
git branch
```

---

### 4️⃣ Make Changes and Push to Your Branch

When you make updates (code, documentation, etc.), commit and push them to **your branch only**:

```bash
git add .
git commit -m "Describe your changes"
git push origin your-branch-name
```

---

### 5️⃣ Submit a Merge Request (MR)

When your work is ready:

1. Go to **GitLab → Merge Requests → New Merge Request**
2. Choose:

   * **Source branch:** your branch (e.g., `alin-raj`)
   * **Target branch:** `main`
3. Add a short description of your work.
4. Click **Create Merge Request**.

Saeed (the Maintainer) will review and merge it into `main`.

---

## ⚠️ Important Rules

| Rule                         | Description                                  |
| ---------------------------- | -------------------------------------------- |
| ❌ Don’t push to `main`       | It’s protected. Use your own branch.         |
| ✅ Work only on your branch   | Keeps code organized and conflict-free.      |
| 🔁 Pull before starting work | Keeps your branch up to date with `main`.    |
| 🧠 Use clear commit messages | Example: `Add GUI connection handler`.       |
| 🧱 Ask for help if unsure    | Saeed will assist with Git or branch issues. |

---

## 🧾 Branch Assignments

| Branch            | Team Member                   | Role                            |
| ----------------- | ----------------------------- | ------------------------------- |
| `saeed-omidvari`  | **Saeed Omidvari**            | Maintainer / System Integration |
| `alin-raj`        | **Alin Raj Rajagopalan Nair** | GUI (GTK / LibSlope)            |
| `mahdi-mohammadi` | **Mohammad Mahdi Mohammadi**  | Networking / TCP Protocol       |
| `ameer-muhammed`  | **Ameer Muhammed**            | Threading & Timing Logic        |
| `reza-rahimi`     | **Mohammad Reza Rahimi**      | Hardware Interface & Simulation |
| `akshay-vastrad`  | **Akshay Vastrad**            | Testing & Validation            |
| `mert-turk`       | **Mert Ali Türk**             | Documentation Lead              |
| `haizon-cruz`     | **Haizon Helet Cruz**         | Architecture & Requirements     |

---

## 🧰 Quick Reference Commands

| Action               | Command                                         |
| -------------------- | ----------------------------------------------- |
| Clone repo           | `git clone <repo-url>`                          |
| Switch branch        | `git checkout <branch>`                         |
| See all branches     | `git branch -a`                                 |
| Pull updates         | `git pull origin <branch>`                      |
| Commit changes       | `git commit -m "message"`                       |
| Push changes         | `git push origin <branch>`                      |
| Create Merge Request | GitLab → **Merge Requests → New Merge Request** |

---

## 💡 Tips

* Use **VS Code** or an IDE with Git integration.
* Run `git status` often to see your changes.
* Communicate in the WhatsApp group before pushing big changes.

---

### Maintainer Contact

📧 **Saeed Omidvari – 41490**
🧠 Hochschule Bremerhaven – Master of Embedded Systems Design

---

© 2025 — Team MNG, Hochschule Bremerhaven
**All rights reserved.**
