# 🔄 Branch Sync Guide

**Author:** Saeed Omidvari — Measurement Network Gateway (MNG)

---

## 🧭 Why this matters

To avoid conflicts and make sure everyone works on the latest version of the project, you should **always update your personal branch with the latest changes from `main`** before starting new work or committing.

This keeps our repository consistent and prevents merge problems when you submit your Merge Request (MR).

---

## 🧩 Option 1 – Merge (safe and easy)

This method merges the latest `main` changes into your branch. It’s simple and safe for everyone.

```bash
git checkout your-branch-name
git fetch origin
git merge origin/main
git push origin your-branch-name
```

✅ Recommended for beginners — preserves full history and minimizes risk.

---

## 🧠 Option 2 – Rebase (clean and professional)

This method re-applies your commits on top of the latest `main` commits, keeping a linear history.

```bash
git checkout your-branch-name
git fetch origin
git rebase origin/main
# If conflicts appear:
#   fix them → git add .
#   continue → git rebase --continue
git push origin your-branch-name --force-with-lease
```

✅ Recommended for experienced users — clean and organized history.
⚠️ Use `--force-with-lease` **only on your own branch**, never on `main`.

---

## 🕓 When to sync

* Before starting any new work on your branch.
* After someone else merges changes into `main`.
* Before opening a Merge Request.

Doing this regularly keeps your branch conflict-free and ensures a smooth merge process later.

---

## 💡 Tips

* Run `git status` often to see what’s going on.
* If you’re unsure, always choose **merge** — it’s safer.
* Ask for help in the team chat if you see conflicts.

---

**Thanks for keeping the project clean and synced! 🙌**

