# Resume Agent Kit

A guided, agent-friendly system for initializing a reusable resume workspace, harvesting context from project repos, and generating Jake-style LaTeX resumes tailored to specific job descriptions.

This kit is designed to work with agents like Claude Code or Codex, but the workflow itself is agent-agnostic.

## What This Repo Contains

- `install.md`: setup instructions for the agent
- `commands/initialize-resume-system.md`: one-time onboarding flow
- `commands/collect-project-context.md`: repo-level context collection flow
- `commands/optimize-resume.md`: normal job-specific resume optimization flow
- `templates/JAKES_RESUME_TEMPLATE.tex`: reusable Jake-style LaTeX template

## Setup Prompt

Paste this into Claude Code or Codex:

```text
Set up https://github.com/neelb1/resume-kit for me.

Read `install.md` first, then follow it exactly.

After that:
1. Use `commands/initialize-resume-system.md` to set up my resume workspace.
2. Tell me exactly what files I need to add and where to put them.
3. Make the setup guided and step-by-step, not abstract.
4. After onboarding, tell me which project repos I should open next and use `commands/collect-project-context.md` inside those repos.
5. Save repo-level context so it can be reused later.
6. Once onboarding and repo-context collection are done, use `commands/optimize-resume.md` for normal job-specific tailoring.
7. Default to the Jake LaTeX template in `templates/JAKES_RESUME_TEMPLATE.tex`.
8. When generating a resume, write a `.tex` file first and compile a PDF by default if LaTeX is available.

Keep the process highly guided. At each stage, tell me exactly what to do next, which folder I should be in, and which command I should run after that.
```

Repository URL: `https://github.com/neelb1/resume-kit`

## High-Level Flow

1. Run initialization once in your main resume workspace.
2. Add your base resume, old resumes, coursework, brag docs, and any strong supporting materials.
3. Run project-context collection inside your most important repos.
4. Return to the main workspace and run the optimize command against a specific JD.

## Why This Exists

Most resumes undersell technical depth because the resume file itself is too thin. This kit fixes that by collecting context from:

- existing resumes
- old tailored versions
- brag docs
- coursework
- project writeups
- actual source repositories

That lets the optimizer sell the user harder while still staying grounded in real experience.
