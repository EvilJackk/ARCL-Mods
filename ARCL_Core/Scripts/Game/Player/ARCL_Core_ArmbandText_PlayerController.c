modded class SCR_PlayerController
{

	protected static const ref array<string> ARCL_Core_ARMBAND_TEXT_FIND = {
		"gloves section",
		"Gloves section",
		"GLOVES SECTION",
		"glove section",
		"Glove section",
		"GLOVE SECTION"
	};

	protected static const ref array<string> ARCL_Core_ARMBAND_TEXT_REPLACE = {
		"Equipment section",
		"Equipment section",
		"EQUIPMENT SECTION",
		"Equipment section",
		"Equipment section",
		"EQUIPMENT SECTION"
	};

	override protected void FTA_BuildMessageForStage(int stage, out string title, out string body)
	{
		super.FTA_BuildMessageForStage(stage, title, body);

		title = ARCL_Core_ApplyArmbandTextFix(title);
		body = ARCL_Core_ApplyArmbandTextFix(body);
	}

	protected string ARCL_Core_ApplyArmbandTextFix(string text)
	{
		if (text.IsEmpty())
			return text;

		int count = ARCL_Core_ARMBAND_TEXT_FIND.Count();
		if (ARCL_Core_ARMBAND_TEXT_REPLACE.Count() < count)
			count = ARCL_Core_ARMBAND_TEXT_REPLACE.Count();

		for (int i = 0; i < count; i++)
		{
			text.Replace(ARCL_Core_ARMBAND_TEXT_FIND[i], ARCL_Core_ARMBAND_TEXT_REPLACE[i]);
		}

		return text;
	}
}
